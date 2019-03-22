# -*- coding: utf-8 -*-
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import paho.mqtt.client as mqtt
import time
import folium
from folium import features
import selenium
from selenium import webdriver

#by default, subscribe to node 16
MQTT_TOPIC = '/ULB/BA2/0'
MQTT_SERVER = 'broker.hivemq.com'
MQTT_SERVERPORT = 1883
driver = webdriver.Chrome("C:/Users/Hassan/Desktop/Gehol-bot/chromedriver.exe")

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.    
    client.subscribe(MQTT_TOPIC)

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    a = str(msg.payload.decode("utf-8")).split('/')

    if len(a) > 2:
        """
        Présence de gps
        """
        try :
            val = a[0].split(',')
            latitude.append(float(val[0]))
            longitude.append(float(val[1]))
        except:
            print(val + ' cannot be converted in a float !')
            
    if a[-2]!='finger??':
        """
        BPM
        """
        try:
            #print(msg.topic + " " + a)
            bpm = a[-2].split(',')[1]
            val=float(bpm) #the msg payload should be better parsed, not
            bpm_xdata.append(time.time()-tstart)
            bpm_ydata.append(val)
        except:
            print('bpm not calculated')
    """
    PAS
    """

    try:
            
        pas = a[-1].split(',')[1]
        val=float(pas) #the msg payload should be better parsed, not
        pas_xdata.append(time.time()-tstart)
        pas_ydata.append(val)
    except:
        print(pas +' cannot be converted in a float !')
    

#Function to init the matplotlib plot
def init_bpm():
    ax_bpm.set_ylim(0, 200)   #internal temperature between 20 & 80 degrees
    ax_bpm.set_xlim(0, 600)  #by default 10min of display (change here if you want a larger window :-)
    del bpm_xdata[:]
    del bpm_ydata[:]
    line_bpm.set_data(bpm_xdata, bpm_ydata)
    return line_bpm,        

#Function execute to animate the matplotlib plot/update data
def run_bpm(data):
    # update the data
    if bpm_xdata:
        xmin, xmax = ax_bpm.get_xlim()
        if bpm_xdata[-1]>xmax: #move x axes limit by 1 minute 
            d=60  #move 1minute axes
            ax_bpm.set_xlim(xmin+d,xmax+d)
            while bpm_xdata[0]<xmin+d: #remove old data
                bpm_xdata.pop(0)
                bpm_xdata.pop(0)
        line_bpm.set_data(bpm_xdata, bpm_ydata)    
    return line_bpm,

#Function to init the matplotlib plot
def init_pas():
    ax_pas.set_ylim(0, 200)   #internal temperature between 20 & 80 degrees
    ax_pas.set_xlim(0, 600)  #by default 10min of display (change here if you want a larger window :-)
    del pas_xdata[:]
    del pas_ydata[:]
    line_pas.set_data(pas_xdata, pas_ydata)
    return line_pas,        

#Function execute to animate the matplotlib plot/update data
def run_pas(data):
    # update the data
    if pas_xdata:
        xmin, xmax = ax_pas.get_xlim()
        if pas_xdata[-1]>xmax: #move x axes limit by 1 minute 
            d=60  #move 1minute axes
            ax_pas.set_xlim(xmin+d,xmax+d)
            while pas_xdata[0]<xmin+d: #remove old data
                pas_xdata.pop(0)
                pas_ydata.pop(0)
        line_pas.set_data(pas_xdata, pas_ydata)    
    return line_pas,    
    
tstart=time.time()  #application time start 
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(MQTT_SERVER, MQTT_SERVERPORT, 60)

fig_bpm, ax_bpm = plt.subplots()
plt.title('BPM')
plt.xlabel('Time (sec)')
plt.ylabel(r'BPM')
fig_pas, ax_pas = plt.subplots()
plt.title('PAS au cours du temps')
plt.xlabel('Time (sec)')
plt.ylabel(r'PAS')
line_bpm, = ax_bpm.plot([], [], lw=2)
line_pas, = ax_pas.plot([], [], lw=2)
ax_bpm.grid()
ax_pas.grid()
pas_xdata = []
bpm_xdata = []
pas_ydata = []
bpm_ydata = []
latitude = []
longitude = []

#matplotlib animation update every second (1000ms)
ani_bpm = animation.FuncAnimation(fig_bpm, run_bpm, interval=1000,
                              repeat=True, init_func=init_bpm)
ani_pas = animation.FuncAnimation(fig_pas, run_pas, interval=1000,
                              repeat=True, init_func=init_pas)
if len(latitude) > 1:
    m = folium.Map([50.812481, 4.382950], zoom_start=20) #on commence au niveau du Square Groupe G
    my_PolyLine = folium.PolyLine(locations=list(zip(latitude, longitude)),weight=3)
    m.add_children(my_PolyLine)
    folium.Marker([latitude[0], longitude[0]], popup='<b>Point de départ</b>', tooltip=tooltip).add_to(m)
    m.save("/Users/raniacharkaoui/Documents/mapselenium.html")
    driver.get("file:///Users/raniacharkaoui/Documents/mapselenium.html")
    time.sleep(5)

plt.autoscale() 
client.loop_start()                              
plt.show()



