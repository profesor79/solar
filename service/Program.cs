﻿// See https://aka.ms/new-console-template for more information
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
var topicList = new List<string>() { "gridVoltage",        

"PvPower",

  "gridFrequency",      //1
  "outputVoltage",      //2
  "outputFrequency",    //3
  "outputPowerApparent",//4
  "outputPowerActive",  //5
  "outputLoadPercent",  //6
  "busVoltage",         //7
  "batteryVoltage",//8
  "batteryChargingCurrent",//9
  "batteryCapacity",//10
  "temperature",//11
  "pvBatteryCurrent",//12
  "pvInputVoltage",//13
  };
        // Create the client object
//        IManagedMqttClient _mqttClient = new MqttFactory().CreateManagedMqttClient();

        // Create client options object
        var clientid ="arduinoRelay";
        var ip ="192.168.1.99";
        System.Console.WriteLine(clientid);
        System.Console.WriteLine(ip);


        // Create the client object
        IManagedMqttClient _mqttClient = new MqttFactory().CreateManagedMqttClient();

        // Create client options object
        MqttClientOptionsBuilder builder = new MqttClientOptionsBuilder()
                                                .WithClientId(clientid)
                                                .WithTcpServer(ip);
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

var lines = returnData.Split(
    new string[] { "\r\n", "\r", "\n" },
    StringSplitOptions.None
);

foreach(var l in lines)
{


var msg = DateTime.Now.ToString("HH:mm:ss.fffff") + " : " + l.ToString();
                    Console.Write(msg);
                    var split = l.Split(':');
                    //File.AppendAllText(path, msg);

                    try
                    {
                        await _mqttClient.EnqueueAsync($"homeassistant/sensor/greg_{split[0]}", split[1]);

                        if (DateTime.UtcNow > lastConfigSent.AddSeconds(300))
                        {                            
                            foreach(var item in topicList)
                            {
                                await publishConfigAsync(item);
                            }
                            
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
