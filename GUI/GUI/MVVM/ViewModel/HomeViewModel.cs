using GUI.Core;
using Newtonsoft.Json;
using System;
using System.Threading.Tasks;
using System.Windows.Media;
using System.Windows.Threading;
using System.Windows.Input;

namespace GUI.MWM.ViewModel
{
    internal class HomeViewModel : ObservableObject
    {
        private DispatcherTimer _refreshTimer;

        public HomeViewModel()
        {
            // Initialize commands
            ToggleValveCommand = new RelayCommand(async (o) => await ToggleValveAsync(o));
            ToggleHydrantCommand = new RelayCommand(async (o) => await ToggleHydrantAsync(o));
            ToggleCycleCommand = new RelayCommand(async (o) => await ToggleCycleAsync(o));
            RefreshCommand = new RelayCommand(async (o) => await LoadConfigurationAsync());

            // Initialize timer for periodic updates
            _refreshTimer = new DispatcherTimer();
            _refreshTimer.Tick += async (s, e) => await LoadConfigurationAsync();
            _refreshTimer.Interval = TimeSpan.FromSeconds(20);
            _refreshTimer.Start();

            // Initial load
            _ = LoadConfigurationAsync();
        }

        #region Properties

        private bool _valve2;
        public bool Valve2 { get => _valve2; set { _valve2 = value; OnPropertyChanged(); } }

        private bool _valve3;
        public bool Valve3 { get => _valve3; set { _valve3 = value; OnPropertyChanged(); } }

        private bool _valve4;
        public bool Valve4 { get => _valve4; set { _valve4 = value; OnPropertyChanged(); } }

        private bool _valve6;
        public bool Valve6 { get => _valve6; set { _valve6 = value; OnPropertyChanged(); } }

        private bool _valve8;
        public bool Valve8 
        { 
            get => _valve8; 
            set 
            { 
                _valve8 = value; 
                OnPropertyChanged(); 
                OnPropertyChanged(nameof(LineS1Brush)); 
            } 
        }

        private bool _valve9;
        public bool Valve9 
        { 
            get => _valve9; 
            set 
            { 
                _valve9 = value; 
                OnPropertyChanged(); 
                OnPropertyChanged(nameof(LineS2Brush)); 
            } 
        }

        private bool _idrStatus;
        public bool IdrStatus { get => _idrStatus; set { _idrStatus = value; OnPropertyChanged(); } }

        private bool _cycleActive;
        public bool CycleActive { get => _cycleActive; set { _cycleActive = value; OnPropertyChanged(); } }

        private bool _rainSensor;
        public bool RainSensor { get => _rainSensor; set { _rainSensor = value; OnPropertyChanged(); OnPropertyChanged(nameof(RainSensorIcon)); } }

        public string RainSensorIcon => RainSensor ? "/MVVM/View/water_drop_blue.png" : "/MVVM/View/water_drop_black.png";

        public Brush LineS1Brush => Valve8 ? Brushes.Blue : Brushes.Red;
        public Brush LineS2Brush => Valve9 ? Brushes.Blue : Brushes.Red;

        private bool _isBusy;
        public bool IsBusy { get => _isBusy; set { _isBusy = value; OnPropertyChanged(); } }

        #endregion

        #region Commands

        public ICommand ToggleValveCommand { get; }
        public ICommand ToggleHydrantCommand { get; }
        public ICommand ToggleCycleCommand { get; }
        public ICommand RefreshCommand { get; }

        #endregion

        private async Task LoadConfigurationAsync()
        {
            if (IsBusy) return;
            IsBusy = true;
            try
            {
                string res = await SocketClient.ExecuteCommandAsync("GET_ALL:AAA;", true);
                if (string.IsNullOrEmpty(res)) return;

                DataTransfer dt = JsonConvert.DeserializeObject<DataTransfer>(res);
                if (dt != null)
                {
                    Valve2 = dt.statuses.valve2;
                    Valve3 = dt.statuses.valve3;
                    Valve4 = dt.statuses.valve4;
                    Valve6 = dt.statuses.valve6;
                    Valve8 = dt.statuses.valve8;
                    Valve9 = dt.statuses.valve9;
                    IdrStatus = dt.statuses.idr;
                    CycleActive = dt.settings.continuosCycleActive;
                    RainSensor = dt.statuses.rainSensor;
                }
            }
            catch (Exception)
            {
                // Silently fail or log for now, as it's a background refresh
            }
            finally
            {
                IsBusy = false;
            }
        }

        private async Task ToggleValveAsync(object parameter)
        {
            if (parameter is string valveId)
            {
                bool newState = GetValveState(valveId);
                string active = newState ? "ON" : "OFF";
                string cmd = $"MANUAL_CONTROL:{valveId}={active};";
                await SocketClient.ExecuteCommandAsync(cmd, false);
                await LoadConfigurationAsync();
            }
        }

        private bool GetValveState(string valveId)
        {
            return valveId switch
            {
                "2" => Valve2,
                "3" => Valve3,
                "4" => Valve4,
                "6" => Valve6,
                "8" => Valve8,
                "9" => Valve9,
                _ => false
            };
        }

        private async Task ToggleHydrantAsync(object parameter)
        {
            string active = IdrStatus ? "ON" : "OFF";
            string cmd = $"MANUAL_CONTROL:IDR={active};";
            await SocketClient.ExecuteCommandAsync(cmd, false);
            await LoadConfigurationAsync();
        }

        private async Task ToggleCycleAsync(object parameter)
        {
            string active = CycleActive ? "ON" : "OFF";
            string cmd = $"CONTINUOS_CYCLE:{active};";
            await SocketClient.ExecuteCommandAsync(cmd, false);
            await LoadConfigurationAsync();
        }
    }
}
