using GUI.Core;
using System.Windows;

namespace GUI.MWM.ViewModel
{
    internal class MainViewModel: ObservableObject
    {

        public RelayCommand HomeViewCommand { get; set; }
        public RelayCommand DiscoveryViewCommand { get; set; }
        public RelayCommand ShowStatusInfoCommand { get; set; }

        public HomeViewModel HomeVM { get; set; }
        public SettingsViewModel settingsVm { get; set; }
        private object _currentView;

        public object CurrentView
        {
            get { return _currentView; }
            set 
            { 
                _currentView = value; 
                OnPropertyChanged();
            }
        }

        private bool _isOnline;
        public bool IsOnline
        {
            get => _isOnline;
            set { _isOnline = value; OnPropertyChanged(); }
        }

        private string _lastSync;
        public string LastSync
        {
            get => _lastSync;
            set { _lastSync = value; OnPropertyChanged(); }
        }

        public MainViewModel()
        {
            HomeVM = new HomeViewModel();
            settingsVm = new SettingsViewModel();
            CurrentView = HomeVM;

            HomeViewCommand = new RelayCommand(o =>
            {
                CurrentView = HomeVM;
            });

            DiscoveryViewCommand = new RelayCommand(o =>
            {
                CurrentView = settingsVm;
            });

            ShowStatusInfoCommand = new RelayCommand(o =>
            {
                MessageBox.Show($"Ultima comunicazione: {LastSync}", "Stato Connessione", MessageBoxButton.OK, MessageBoxImage.Information);
            });

            SocketClient.CommunicationStatusChanged += (success, time) =>
            {
                IsOnline = success;
                LastSync = time.ToString("dd/MM/yyyy HH:mm:ss");
            };
        }

    }
}
