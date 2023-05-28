// See https://aka.ms/new-console-template for more information
using System.Net.Sockets;
using System.Net;
using System.Text;
using System.IO;
using MQTTnet;

using MQTTnet.Extensions.ManagedClient;
using MQTTnet.Client;

using System.Text.Json;

internal class Program
{
    private static async Task Main(string[] args)
    {
        Console.WriteLine("Hello, World!");

        // Create the client object
        IManagedMqttClient _mqttClient = new MqttFactory().CreateManagedMqttClient();

        // Create client options object
        MqttClientOptionsBuilder builder = new MqttClientOptionsBuilder()
                                                .WithClientId("arduinoRelay")
                                                .WithTcpServer("192.168.1.79");
        ManagedMqttClientOptions options = new ManagedMqttClientOptionsBuilder()
                                .WithAutoReconnectDelay(TimeSpan.FromSeconds(60))
                                .WithClientOptions(builder.Build())
                                .Build();

        // Set up handlers
        _mqttClient.ConnectedAsync += _mqttClient_ConnectedAsync;

        _mqttClient.DisconnectedAsync += _mqttClient_DisconnectedAsync;

        _mqttClient.ConnectingFailedAsync += _mqttClient_ConnectingFailedAsync;

        // Connect to the broker
        await _mqttClient.StartAsync(options);

        //Creates a UdpClient for reading incoming data.
        UdpClient receivingUdpClient = new UdpClient(8888);

        //Creates an IPEndPoint to record the IP Address and port number of the sender.
        // The IPEndPoint will allow you to read datagrams sent from any source.
        IPEndPoint RemoteIpEndPoint = new IPEndPoint(IPAddress.Any, 0);
        var lastConfigSent = DateTime.UtcNow.AddSeconds(-290);
        while (true)
        {
            try
            {
                var path = @"c:\code\temp.log";
                // Blocks until a message returns on this socket from a remote host.
                byte[] receiveBytes = receivingUdpClient.Receive(ref RemoteIpEndPoint);

                string returnData = Encoding.ASCII.GetString(receiveBytes);
                if (!string.IsNullOrEmpty(returnData))
                {
                    var msg = DateTime.Now.ToString("HH:mm:ss.fffff") + " : " + returnData.ToString();
                    Console.Write(msg);
                    var split = returnData.Split(':');
                    File.AppendAllText(path, msg);

                    try
                    {
                        await _mqttClient.EnqueueAsync($"homeassistant/sensor/greg_{split[0]}", split[1].TrimEnd('0'));

                        if (DateTime.UtcNow > lastConfigSent.AddSeconds(300))
                        {
                            await publishConfigAsync("WaterTank");
                            await publishConfigAsync("Tank2Pump");
                            await publishConfigAsync("ReturnToTank");
                            await publishConfigAsync("RoofZone1");
                            await publishConfigAsync("RoofZone2");
                            await publishConfigAsync("TemperatureDiff");
                            await publishConfigAsync("WaterPump", true);
                            await publishConfigAsync("SolarPump", true);
                            await publishConfigAsync("SolarPumpPowerOn", true);
                            await publishConfigAsync("SystemPowerOn", true);
                            await publishConfigAsync("WaterPumpPowerOn", true);

                            lastConfigSent = DateTime.UtcNow;
                        }
                    }
                    catch (Exception u)
                    {
                        Console.WriteLine("cant do it:" + returnData);
                        Console.WriteLine(u.Message
                            );
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
            }
        }

        Task _mqttClient_ConnectedAsync(MqttClientConnectedEventArgs arg)
        {
            Console.WriteLine("Connected to broker");
            return Task.CompletedTask;
        };
        Task _mqttClient_DisconnectedAsync(MqttClientDisconnectedEventArgs arg)
        {
            Console.WriteLine("Disconnected from broker");
            return Task.CompletedTask;
        };
        Task _mqttClient_ConnectingFailedAsync(ConnectingFailedEventArgs arg)
        {
            Console.WriteLine("Connection failed check network or broker!");
            return Task.CompletedTask;
        }
        const string configBase = "{\"name\": \"__\",\"unit_of_measurement\": \"\",\"state_topic\": \"homeassistant/sensor/__\",\"icon\": \"mdi:temperature-celsius\" }";
        async Task publishConfigAsync(string name, bool isPump = false)
        {
            var c = configBase.Replace("__", "greg_" + name);
            if (isPump)
            {
                c = c.Replace("mdi:temperature-celsius", "mdi:pump");
            }
            var topic = "homeassistant/sensor/greg_" + name + "/config";
            await _mqttClient.EnqueueAsync(topic, c);
            Console.WriteLine($"{topic}--{c}");
        }
    }
}