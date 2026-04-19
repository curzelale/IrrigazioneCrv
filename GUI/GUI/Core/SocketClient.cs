using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace GUI.Core
{
    public static class SocketClient
    {
        public static event Action<bool, DateTime> CommunicationStatusChanged;

        // Default values, can be made configurable later
        public static string ServerIp { get; set; } = "192.168.178.254";
        public static int ServerPort { get; set; } = 23;

        public static async Task<string> ExecuteCommandAsync(string cmd, bool needResponse)
        {
            string res = string.Empty;
            bool success = false;
            
            if (!IPAddress.TryParse(ServerIp, out IPAddress ipAddr))
            {
                CommunicationStatusChanged?.Invoke(false, DateTime.Now);
                throw new ArgumentException("Invalid IP Address configuration.");
            }

            IPEndPoint remoteEP = new IPEndPoint(ipAddr, ServerPort);

            using (Socket sender = new Socket(ipAddr.AddressFamily, SocketType.Stream, ProtocolType.Tcp))
            {
                try
                {
                    // Set a timeout for connection
                    var connectTask = sender.ConnectAsync(remoteEP);
                    if (await Task.WhenAny(connectTask, Task.Delay(5000)) != connectTask)
                    {
                        throw new TimeoutException("Connection to server timed out.");
                    }

                    byte[] messageSent = Encoding.ASCII.GetBytes(cmd);
                    await sender.SendAsync(new ArraySegment<byte>(messageSent), SocketFlags.None);

                    if (needResponse)
                    {
                        byte[] buffer = new byte[2048]; // Increased buffer size
                        int bytesRead = await sender.ReceiveAsync(new ArraySegment<byte>(buffer), SocketFlags.None);
                        res = Encoding.ASCII.GetString(buffer, 0, bytesRead);
                    }

                    sender.Shutdown(SocketShutdown.Both);
                    success = true;
                }
                catch (Exception ex)
                {
                    // In a real app, we might want to log this or rethrow a custom exception
                    Console.WriteLine($"Socket Error: {ex.Message}");
                    success = false;
                    throw; // Rethrow to let the caller handle UI feedback
                }
                finally
                {
                    CommunicationStatusChanged?.Invoke(success, DateTime.Now);
                    if (sender.Connected)
                    {
                        sender.Close();
                    }
                }
            }
            return res;
        }
    }
}
