
import numpy as np
from scipy import signal
from matplotlib import pyplot as plt
from matplotlib import animation
#np.seterr(all='ignore')

#Set up UDP Server
print("Starting UDP Server")
import socket
UDP_IP = "192.168.88.168"
UDP_PORT = 9999
BUFFER_SIZE = 6
global s
s = socket.socket(socket.AF_INET,socket.SOCK_DGRAM) # UDP
s.bind((UDP_IP, UDP_PORT))  #binds the socket
print 'UDP Server listening...'

#Number of point in X axis
POINTS = 2000

#FIR Filter constants (lowpass and notch)
Fs=500.0
Fnyqst=Fs/2
lowCutFreq=30.0
notchFreqs=[47.0/Fnyqst, 53.0/Fnyqst]
width = 5.0/Fnyqst
ripple = 80.0
N, beta = signal.kaiserord(ripple, width)
tapsLow = signal.firwin(N, lowCutFreq/Fnyqst, window=('kaiser', beta)) #Lowpass filter coefficients
tapsLowNotch=signal.firwin(N, notchFreqs, window=('kaiser', beta))  #Notch filter coeeficients

#IIR filter constants (highpass DC removal)
ECG_ch0_Pvev_DC_Sample = 0
ECG_ch0_Pvev_Sample = 0
NRCOEFF = 0.992

#FIR Filter buffer
filterBuffer = [0]*2500

# First set up the figure, the axis, and the plot element we want to animate
fig = plt.figure()
ax = fig.add_subplot(2,1,1)
ax = plt.axes(xlim=(0, POINTS), ylim=(-300,300), title="ECG Signal", ylabel="Sample data", xlabel="Sample number")
line, = ax.plot([], [], lw=2)


#Setup database connection
# import mysql.connector
# import datetime

# con=mysql.connector.connect(user='root', password ='utukth', database='ecg_data')
# cur=con.cursor()
# date=datetime.datetime.utcnow()
# ecg_data = 99
# sensor_id = 66


#Convert to two's complement
def twos_comp(val, bits):
    """compute the 2's compliment of int value val"""
    if( (val&(1<<(bits-1))) != 0 ):
        val = val - (1<<bits)
    return val
    
#Initialization function: plot the background of each frame
def init():
	#Fill first empty frame
    line.set_data([], [])
    global y
    global x
    x = np.linspace(0, POINTS, POINTS)
    y = [0]*POINTS
    return line,

	
#Animation function. This is called sequentially
def animate(i):
    global y
	
	#Receive 32 samples and filter them
    for i in range(33): 
        data, addr = s.recvfrom(6)
        data=data.encode("hex")
        data=data[4:8]	#Channel 1
        #Convert to two's complement
        if data >= 32768:
    	    data = twos_comp(int(data,16), 16)
        else:
    	    data = int(data,16)
			
        #DC removal first order IIR Filter
        global ECG_ch0_Pvev_DC_Sample
        global ECG_ch0_Pvev_Sample
        temp1 = NRCOEFF * ECG_ch0_Pvev_DC_Sample
        ECG_ch0_Pvev_DC_Sample = (data  - ECG_ch0_Pvev_Sample) + temp1
        ECG_ch0_Pvev_Sample = data
        ECGData_ch0 = ECG_ch0_Pvev_DC_Sample
        
		#Add DC removed sample to buffer
        global filterBuffer
        filterBuffer = np.concatenate((filterBuffer[1:], [ECGData_ch0]), axis=0)
	# print max(filterBuffer);
    # Lowpass filter	
    filteredData= signal.lfilter(tapsLow,1.0,filterBuffer);
	
	# Notch filter
    filteredData= signal.lfilter(tapsLowNotch,1.0,filteredData);
	
	# Median filter for smoothing
    filteredData = signal.medfilt(filteredData,9)
	
	#Store in DB
	# date=datetime.datetime.utcnow()
    # sel=("INSERT INTO ecg (ecg_time_stamp, ecg_data, sensor_id) values ('%s', %d, %d)" % (date, ECGData_ch0, sensor_id))
    # cur.execute(sel)
    # con.commit()
	#Draw line
    line.set_data(x, filteredData[500:] )
    return line,

# call the animator.  blit=True means only re-draw the parts that have changed.
anim = animation.FuncAnimation(fig, animate, init_func=init,
                               frames=1000, interval=2, blit=True)

# save the animation as an mp4.  This requires ffmpeg or mencoder to be
# installed.  The extra_args ensure that the x264 codec is used, so that
# the video can be embedded in html5.  You may need to adjust this for
# your system: for more information, see
# http://matplotlib.sourceforge.net/api/animation_api.html
# anim.save('basic_animation.mp4', fps=30, extra_args=['-vcodec', 'libx264'])

plt.show()
