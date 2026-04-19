using GUI.Core;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Threading.Tasks;
using System.Windows.Input;

namespace GUI.MWM.ViewModel
{
    internal class SettingsViewModel : ObservableObject
    {
        public SettingsViewModel()
        {
            SaveCommand = new RelayCommand(async (o) => await SaveAsync());
            UpdateDateCommand = new RelayCommand(async (o) => await UpdateDateAsync());
            RefreshCommand = new RelayCommand(async (o) => await LoadConfigurationAsync());

            // Initial load
            _ = LoadConfigurationAsync();
        }

        #region Properties

        private int _mode;
        public bool IsAuto { get => _mode == 1; set { if (value) Mode = 1; OnPropertyChanged(); } }
        public bool IsManual { get => _mode == 2; set { if (value) Mode = 2; OnPropertyChanged(); } }
        public bool IsNoControl { get => _mode == 3; set { if (value) Mode = 3; OnPropertyChanged(); } }
        public bool IsOff { get => _mode == 4; set { if (value) Mode = 4; OnPropertyChanged(); } }

        public string ModeName
        {
            get => Mode switch
            {
                1 => "Automatico",
                2 => "Manuale",
                3 => "Senza Controlli",
                4 => "Spento",
                _ => "Sconosciuto"
            };
        }

        public int Mode
        {
            get => _mode;
            set
            {
                _mode = value;
                OnPropertyChanged();
                OnPropertyChanged(nameof(IsAuto));
                OnPropertyChanged(nameof(IsManual));
                OnPropertyChanged(nameof(IsNoControl));
                OnPropertyChanged(nameof(IsOff));
                OnPropertyChanged(nameof(ModeName));
            }
        }

        private string _startTime1;
        public string StartTime1 { get => _startTime1; set { _startTime1 = value; OnPropertyChanged(); } }

        private string _startTime2;
        public string StartTime2 { get => _startTime2; set { _startTime2 = value; OnPropertyChanged(); } }

        private string _onTime1;
        public string OnTime1 { get => _onTime1; set { _onTime1 = value; OnPropertyChanged(); } }

        private string _onTime2;
        public string OnTime2 { get => _onTime2; set { _onTime2 = value; OnPropertyChanged(); } }

        private string _onTime3;
        public string OnTime3 { get => _onTime3; set { _onTime3 = value; OnPropertyChanged(); } }

        private string _onTime4;
        public string OnTime4 { get => _onTime4; set { _onTime4 = value; OnPropertyChanged(); } }

        private string _onTime5;
        public string OnTime5 { get => _onTime5; set { _onTime5 = value; OnPropertyChanged(); } }

        private string _onTime6;
        public string OnTime6 { get => _onTime6; set { _onTime6 = value; OnPropertyChanged(); } }

        private string _onTime7;
        public string OnTime7 { get => _onTime7; set { _onTime7 = value; OnPropertyChanged(); } }

        private string _cycleTime;
        public string CycleTime { get => _cycleTime; set { _cycleTime = value; OnPropertyChanged(); } }

        private bool _rainSensorActive;
        public bool RainSensorActive { get => _rainSensorActive; set { _rainSensorActive = value; OnPropertyChanged(); } }

        private string _controllerDate;
        public string ControllerDate { get => _controllerDate; set { _controllerDate = value; OnPropertyChanged(); } }

        private bool _isBusy;
        public bool IsBusy { get => _isBusy; set { _isBusy = value; OnPropertyChanged(); } }

        #endregion
        #region Commands
        public ICommand SaveCommand { get; }
        public ICommand UpdateDateCommand { get; }
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
                    Mode = dt.settings.mode;
                    StartTime1 = dt.settings.startTime1;
                    StartTime2 = dt.settings.startTime2;
                    OnTime1 = SecondsToTime(dt.settings.onTime1);
                    OnTime2 = SecondsToTime(dt.settings.onTime2);
                    OnTime3 = SecondsToTime(dt.settings.onTime3);
                    OnTime4 = SecondsToTime(dt.settings.onTime4);
                    OnTime5 = SecondsToTime(dt.settings.onTime5);
                    OnTime6 = SecondsToTime(dt.settings.onTime6);
                    OnTime7 = SecondsToTime(dt.settings.onTime7);
                    CycleTime = SecondsToTime(dt.settings.cycleIntertime);
                    RainSensorActive = dt.settings.rainSensorActive;
                    ControllerDate = dt.settings.currentControllerDate;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error loading configuration: {ex.Message}");
            }
            finally
            {
                IsBusy = false;
            }
        }

        private async Task SaveAsync()
        {
            if (IsBusy) return;
            IsBusy = true;
            try
            {
                var settings = new Dictionary<string, string>
                {
                    { "MODE", Mode.ToString() },
                    { "STARTTIME1", StartTime1 },
                    { "STARTTIME2", StartTime2 },
                    { "ONTIME1", TimeToSeconds(OnTime1) },
                    { "ONTIME2", TimeToSeconds(OnTime2) },
                    { "ONTIME3", TimeToSeconds(OnTime3) },
                    { "ONTIME4", TimeToSeconds(OnTime4) },
                    { "ONTIME5", TimeToSeconds(OnTime5) },
                    { "ONTIME6", TimeToSeconds(OnTime6) },
                    { "ONTIME7", TimeToSeconds(OnTime7) },
                    { "RAIN_SENSOR", RainSensorActive ? "ACTIVE" : "DEACTIVE" },
                    { "CONTINUOS_CYCLE_INTERTIME", TimeToSeconds(CycleTime) }
                };

                foreach (var kv in settings)
                {
                    await SocketClient.ExecuteCommandAsync($"SET_PARAM:{kv.Key}#{kv.Value};", true);
                }
                await SocketClient.ExecuteCommandAsync("STORE_CHANGES:AAA;", true);
                
                await LoadConfigurationAsync();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error saving settings: {ex.Message}");
            }
            finally
            {
                IsBusy = false;
            }
        }

        private async Task UpdateDateAsync()
        {
            try
            {
                string dateStr = DateTime.Now.ToString("MMM dd yyyy-HH:mm:ss", CultureInfo.CreateSpecificCulture("en-US"));
                await SocketClient.ExecuteCommandAsync($"SET_DATE:{dateStr};", true);
                await LoadConfigurationAsync();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error updating date: {ex.Message}");
            }
        }

        private string TimeToSeconds(string timeString)
        {
            if (string.IsNullOrEmpty(timeString) || !timeString.Contains(":")) return "0";
            try
            {
                string[] times = timeString.Split(':');
                int minutes = int.Parse(times[0]);
                int seconds = int.Parse(times[1]);
                return ((minutes * 60) + seconds).ToString();
            }
            catch { return "0"; }
        }

        private string SecondsToTime(int seconds)
        {
            return (seconds / 60) + ":" + (seconds % 60).ToString("D2");
        }
    }
}
