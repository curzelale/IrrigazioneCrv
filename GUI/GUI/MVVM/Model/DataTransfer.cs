using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GUI.Core
{
    public class DataTransfer
    {
        public Statuses statuses { get; set; }
        public Settings settings { get; set; }
    }

    public class Settings
    {
        public int mode { get; set; }
        public String startTime1 { get; set; }
        public String startTime2 { get; set; }
        public String currentControllerDate { get; set; }
        public int onTime1 { get; set; }
        public int onTime2 { get; set; }
        public int onTime3 { get; set; }
        public int onTime4 { get; set; }
        public int onTime5 { get; set; }

        //SPRUZZATORI
        public int onTime6 { get; set; }
        public int onTime7 { get; set; }
        public bool continuosCycleActive { get; set; }
        public int cycleIntertime { get; set; }
        public bool rainSensorActive { get; set; }
    }

    public class Statuses
    {
        public bool valve1 { get; set; }
        public bool valve2 { get; set; }
        public bool valve3 { get; set; }
        public bool valve4 { get; set; }
        public bool valve5 { get; set; }
        public bool valve6 { get; set; }
        public bool valve7 { get; set; }
        public bool valve8 { get; set; }
        public bool valve9 { get; set; }
        public bool idr{ get; set; }
        public bool rainSensor { get; set; }
    }
}
