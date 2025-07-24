using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Threading;

namespace PloutonLogViewer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private const string LanguageSettingFile = "language.cfg";
        private readonly PhysicalMemoryReader _memoryReader;
        private readonly DispatcherTimer _logPollTimer;
        private ulong _physicalAddress;
        private uint _lastReadOffset;
        private bool _isMonitoring;
        private bool _isReaderInitialized = false;
        private const uint LogBufferSize = 1 * 1024 * 1024; // 1MB, must match driver

        public MainWindow()
        {
            InitializeComponent();
            LoadLanguage();

            _memoryReader = new PhysicalMemoryReader();
            _logPollTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromMilliseconds(50)
            };
            _logPollTimer.Tick += LogPollTimer_Tick;

            UpdateStatus(GetResource("StatusNotStarted"), false);
            StartPauseButton.IsEnabled = false;
        }

        #region Language Switching

        private string GetResource(string key)
        {
            var resource = Application.Current.FindResource(key);
            return resource as string ?? key;
        }

        private string GetResourceFormat(string key, params object[] args)
        {
            string format = GetResource(key);
            return string.Format(CultureInfo.CurrentUICulture, format, args);
        }

        private void LoadLanguage()
        {
            string languageCode = "en-US"; // Default
            if (File.Exists(LanguageSettingFile))
            {
                languageCode = File.ReadAllText(LanguageSettingFile);
            }
            SwitchLanguage(languageCode);

            foreach (ComboBoxItem item in LanguageComboBox.Items)
            {
                if (item.Tag.ToString() == languageCode)
                {
                    LanguageComboBox.SelectedItem = item;
                    break;
                }
            }
        }

        private void SwitchLanguage(string languageCode)
        {
            var dict = new ResourceDictionary();
            switch (languageCode)
            {
                case "zh-CN":
                    dict.Source = new Uri("Resources/StringResources.zh-CN.xaml", UriKind.Relative);
                    break;
                default:
                    dict.Source = new Uri("Resources/StringResources.en-US.xaml", UriKind.Relative);
                    break;
            }

            Application.Current.Resources.MergedDictionaries.Clear();
            Application.Current.Resources.MergedDictionaries.Add(dict);

            try
            {
                File.WriteAllText(LanguageSettingFile, languageCode);
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"Failed to save language setting: {ex.Message}");
            }
            
            // Refresh button content if state is already set
            if (StartPauseButton.IsEnabled)
            {
                StartPauseButton.Content = _isMonitoring ? GetResource("PauseButton") : GetResource("StartButton");
            }
        }

        private void LanguageComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (LanguageComboBox.SelectedItem is ComboBoxItem selectedItem && selectedItem.Tag is string languageCode)
            {
                SwitchLanguage(languageCode);
            }
        }

        #endregion

        #region UI Event Handlers

        private async void GetAddressButton_Click(object sender, RoutedEventArgs e)
        {
            GetAddressButton.IsEnabled = false;
            UpdateStatus(GetResource("StatusGettingAddress"), false);

            if (!_isReaderInitialized)
            {
                try
                {
                    _memoryReader.Initialize();
                    _isReaderInitialized = true;
                }
                catch (Exception ex)
                {
                    UpdateStatus(GetResource("StatusDriverError") + ": " + ex.Message, true);
                    GetAddressButton.IsEnabled = true;
                    return;
                }
            }

            try
            {
                var (addressHex, error) = await GetPloutonLogAddressAsync();
                if (error != null)
                {
                    UpdateStatus(GetResource("StatusMemoryReadFailed") + ": " + error, true);
                }
                else
                {
                    AddressTextBox.Text = addressHex;
                    // TextChanged event will call ParseAndSetAddress
                }
            }
            catch (Exception ex)
            {
                UpdateStatus(GetResource("StatusMemoryReadFailed") + ": " + ex.Message, true);
            }
            finally
            {
                GetAddressButton.IsEnabled = true;
            }
        }

        private void AddressTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            ParseAndSetAddress(AddressTextBox.Text);
        }

        private void StartPauseButton_Click(object sender, RoutedEventArgs e)
        {
            _isMonitoring = !_isMonitoring;
            if (_isMonitoring)
            {
                if (!_isReaderInitialized)
                {
                    try
                    {
                        _memoryReader.Initialize();
                        _isReaderInitialized = true;
                    }
                    catch (Exception ex)
                    {
                        UpdateStatus(GetResource("StatusDriverError") + ": " + ex.Message, true);
                        _isMonitoring = false;
                        return;
                    }
                }
                _logPollTimer.Start();
                UpdateStatus(GetResource("StatusMonitoring"));
                StartPauseButton.Content = GetResource("PauseButton");
            }
            else
            {
                _logPollTimer.Stop();
                UpdateStatus(GetResource("StatusPaused"), false);
                StartPauseButton.Content = GetResource("StartButton");
            }
        }

        private void ClearButton_Click(object sender, RoutedEventArgs e)
        {
            LogTextBox.Clear();
            _lastReadOffset = 0; // Reset read position as well
        }

        private void CopyButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (!string.IsNullOrEmpty(LogTextBox.Text))
                {
                    Clipboard.SetText(LogTextBox.Text);
                    UpdateStatus(GetResource("StatusLogsCopied"), false);
                }
            }
            catch (Exception ex)
            {
                UpdateStatus(GetResource("StatusCopyFailed") + ": " + ex.Message, true);
            }
        }

        private void SaveButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (string.IsNullOrEmpty(LogTextBox.Text)) return;

                var saveFileDialog = new Microsoft.Win32.SaveFileDialog
                {
                    FileName = $"PloutonLog_{DateTime.Now:yyyyMMdd_HHmmss}.txt",
                    Filter = "Text Files (*.txt)|*.txt|All Files (*.*)|*.*",
                    Title = GetResource("SaveLogTitle")
                };

                if (saveFileDialog.ShowDialog() == true)
                {
                    File.WriteAllText(saveFileDialog.FileName, LogTextBox.Text);
                    UpdateStatus(GetResourceFormat("StatusLogSaved", saveFileDialog.FileName), false);
                }
            }
            catch (Exception ex)
            {
                UpdateStatus(GetResource("StatusSaveFailed") + ": " + ex.Message, true);
            }
        }

        private void CopyAddressButton_Click(object sender, RoutedEventArgs e)
        {
            if (_physicalAddress != 0)
            {
                string addressHex = $"0x{_physicalAddress:X}";
                Clipboard.SetText(addressHex);
            }
        }

        private void CopyStatusButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Clipboard.SetText(StatusText.Text);
                // Optionally provide feedback to the user
            }
            catch (Exception ex)
            {
                // Handle exceptions from accessing the clipboard
                UpdateStatus(GetResource("StatusCopyFailed") + ": " + ex.Message, true);
            }
        }

        protected override void OnClosed(EventArgs e)
        {
            _logPollTimer?.Stop();
            _memoryReader?.Dispose();
            base.OnClosed(e);
        }

        #endregion

        #region Core Logic

        private void ParseAndSetAddress(string addressHex)
        {
            if (string.IsNullOrWhiteSpace(addressHex))
            {
                StartPauseButton.IsEnabled = false;
                return;
            }
            
            if (addressHex.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
            {
                addressHex = addressHex.Substring(2);
            }

            if (ulong.TryParse(addressHex, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out _physicalAddress))
            {
                UpdateStatus(GetResource("StatusAddressSuccess") + $"0x{_physicalAddress:X}", false);
                _lastReadOffset = 0; // Reset read position every time a new valid address is set
                StartPauseButton.IsEnabled = true;
                
                // Read existing logs once to show boot-time logs
                ReadAndAppendLogs();
            }
            else
            {
                UpdateStatus(GetResource("StatusAddressInvalid"), true);
                StartPauseButton.IsEnabled = false;
            }
        }
        
        private void LogPollTimer_Tick(object? sender, EventArgs e)
        {
            ReadAndAppendLogs();
        }

        private void ReadAndAppendLogs()
        {
            if (_physicalAddress == 0) return;

            try
            {
                byte[] logData = new byte[LogBufferSize];
                _memoryReader.ReadPhysicalMemory(_physicalAddress, logData);

                // The log from SMM is a stream of ASCII characters, terminated by one or more nulls.
                // We treat the entire buffer as a single string and find its end.
                string fullLogText = Encoding.ASCII.GetString(logData).TrimEnd('\0');

                if (fullLogText.Length > _lastReadOffset)
                {
                    // Standard case: new text has been appended.
                    string newText = fullLogText.Substring((int)_lastReadOffset);
                    LogTextBox.AppendText(newText);
                    LogTextBox.ScrollToEnd();
                    _lastReadOffset = (uint)fullLogText.Length;
                }
                else if (fullLogText.Length < _lastReadOffset)
                {
                    // Wrap-around case: the buffer was cleared and started from the beginning.
                    // The new content is the entire current buffer.
                    LogTextBox.AppendText(fullLogText);
                    LogTextBox.ScrollToEnd();
                    _lastReadOffset = (uint)fullLogText.Length;
                }
                // If fullLogText.Length == _lastReadOffset, do nothing.
            }
            catch (Exception ex)
            {
                _logPollTimer.Stop();
                UpdateStatus(GetResource("StatusMemoryReadFailed") + ": " + ex.Message, true);
                StartPauseButton.Content = GetResource("StartButton");
                _isMonitoring = false;
            }
        }
        
        private void UpdateStatus(string message, bool isError = false)
        {
            StatusText.Text = message;
            StatusText.Foreground = isError ? Brushes.Red : Brushes.Gray;
            CopyStatusButton.Visibility = isError ? Visibility.Visible : Visibility.Collapsed;
        }

        private Task<(string? Address, string? Error)> GetPloutonLogAddressAsync()
        {
            return Task.Run(() =>
            {
                if (!NativeMethods.EnablePrivilege("SeSystemEnvironmentPrivilege"))
                {
                    return ((string?)null, GetResource("StatusPrivilegeFailed"));
                }

                const string guid = "{71245e36-47a7-4458-8588-74a4411b9332}";
                const string name = "PloutonLogAddress";
                const uint bufferSize = 8; // ulong is 8 bytes
                var buffer = new byte[bufferSize];

                // This code runs on a background thread.
                var result = NativeMethods.GetFirmwareEnvironmentVariable(name, guid, buffer, bufferSize);
                
                if (result == 0)
                {
                    int errorCode = Marshal.GetLastWin32Error();
                    if (errorCode == 203) // ERROR_ENVVAR_NOT_FOUND
                    {
                        return ((string?)null, GetResource("StatusUefiVarNotFound"));
                    }
                    string errorMessage = new Win32Exception(errorCode).Message;
                    return ((string?)null, GetResourceFormat("StatusUefiError", errorCode, errorMessage));
                }

                if (result != bufferSize)
                {
                    return ((string?)null, GetResourceFormat("StatusUefiVarSizeError", result, bufferSize));
                }

                ulong physicalAddress = BitConverter.ToUInt64(buffer, 0);
                return (physicalAddress.ToString("X"), (string?)null);
            });
        }
        #endregion
    }

    internal static class NativeMethods
    {
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        internal static extern uint GetFirmwareEnvironmentVariable(
            string lpName,
            string lpGuid,
            [Out] byte[] pBuffer,
            uint nSize);

        [DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool LookupPrivilegeValue(string? lpSystemName, string lpName, out LUID lpLuid);

        [DllImport("advapi32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool AdjustTokenPrivileges(IntPtr TokenHandle, [MarshalAs(UnmanagedType.Bool)] bool DisableAllPrivileges, ref TOKEN_PRIVILEGES NewState, uint Zero, IntPtr Null1, IntPtr Null2);

        [DllImport("advapi32.dll", SetLastError = true)]
        private static extern bool OpenProcessToken(IntPtr ProcessHandle, uint DesiredAccess, out IntPtr TokenHandle);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr GetCurrentProcess();
        
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool CloseHandle(IntPtr hObject);

        private const uint TOKEN_ADJUST_PRIVILEGES = 0x0020;
        private const uint TOKEN_QUERY = 0x0008;

        [StructLayout(LayoutKind.Sequential)]
        private struct LUID
        {
            public uint LowPart;
            public int HighPart;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct TOKEN_PRIVILEGES
        {
            public uint PrivilegeCount;
            public LUID Luid;
            public uint Attributes;
        }

        internal static bool EnablePrivilege(string privilegeName)
        {
            var processHandle = GetCurrentProcess();
            if (!OpenProcessToken(processHandle, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, out var tokenHandle))
            {
                return false;
            }

            if (!LookupPrivilegeValue(null, privilegeName, out var luid))
            {
                CloseHandle(tokenHandle);
                return false;
            }

            var newState = new TOKEN_PRIVILEGES
            {
                PrivilegeCount = 1,
                Luid = luid,
                Attributes = 2 // SE_PRIVILEGE_ENABLED
            };

            bool result = AdjustTokenPrivileges(tokenHandle, false, ref newState, 0, IntPtr.Zero, IntPtr.Zero);
            CloseHandle(tokenHandle);
            return result;
        }
    }
} 