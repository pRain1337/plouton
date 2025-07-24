using System;
using System.Diagnostics;
using System.Security.Principal;
using System.Windows;

namespace PloutonLogViewer
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        public App()
        {
            this.DispatcherUnhandledException += App_DispatcherUnhandledException;
        }

        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            var principal = new WindowsPrincipal(WindowsIdentity.GetCurrent());
            if (!principal.IsInRole(WindowsBuiltInRole.Administrator))
            {
                try
                {
                    var exeName = Process.GetCurrentProcess().MainModule?.FileName;
                    if (exeName != null)
                    {
                        var startInfo = new ProcessStartInfo(exeName)
                        {
                            Verb = "runas",
                            UseShellExecute = true
                        };
                        Process.Start(startInfo);
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show("This application requires administrator privileges. Please right-click the application and select 'Run as administrator'.\n\nFailed to restart automatically:\n" + ex.Message, "Administrator Privileges Required", MessageBoxButton.OK, MessageBoxImage.Error);
                }
                
                Application.Current.Shutdown();
                return;
            }
        }

        void App_DispatcherUnhandledException(object sender, System.Windows.Threading.DispatcherUnhandledExceptionEventArgs e)
        {
            // Create a formatted error message
            string errorMessage = $"An unhandled exception occurred: \n\n{e.Exception.Message}\n\nStack Trace:\n{e.Exception.StackTrace}";

            // Show the error in a message box
            MessageBox.Show(errorMessage, "Fatal Error", MessageBoxButton.OK, MessageBoxImage.Error);

            // Mark the exception as handled to prevent the application from closing immediately
            e.Handled = true;

            // Optionally, shut down the application
            Application.Current.Shutdown();
        }
    }
} 