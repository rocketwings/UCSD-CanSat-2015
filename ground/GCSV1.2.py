# ---------------------------------------------------------------------------------------
# Libraries
# ---------------------------------------------------------------------------------------
import Tkinter as tk
from datetime import datetime
import multiprocessing
from multiprocessing import Queue as Q
import ttk
import time
import serial
import matplotlib
import math
import matplotlib.animation as animation
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2TkAgg
from matplotlib import style
from matplotlib import pyplot as plt
from matplotlib import dates
import Queue
import threading
from time import sleep
import sys
import tkMessageBox

# from matplotlib.figure import Figure
# from Tkinter import ttk
# ---------------------------------------------------------------------------------------
# globals
# ---------------------------------------------------------------------------------------
LARGE_FONT = ("Verdana", 12)
MEDIUM_FONT = ("Verdana", 10)
SMALL_FONT = ("Verdana", 8)

ALTITUDE_COLOR = "#00EE76"
PRESSURE_COLOR = "#00EEEE"
TEMPERATURE_COLOR = "#FF6103"
SPEED_COLOR = "#FF3030"
VOLTAGE_COLOR = "#EE00EE"
GPSSPD_COLOR = "#FFC125"

PADDING_COLOR = "#404040"
PADDING_COLOR1 = "#404040"
BACKGROUND_COLOR4 = "#00E6A7"
BACKGROUND_COLOR3 = "#00C78C"
BACKGROUND_COLOR = "#0F0F0F"
BACKGROUND_COLOR1 = "#C6E2FF"
BACKGROUND_MENU_COLOR = "#0F0F0F"
GRAPH_BG = "#404040"
GRAPH_BG1 = "#6C7B8B"
TEXT_COLOR = "#FFFFFF"

LogName = "testdata3.txt"

NumberOfPoints = 20

matplotlib.use("TkAgg")
plt.style.use("ggplot")
f = plt.figure(facecolor=BACKGROUND_COLOR)
# a = f.add_subplot(111)

GraphParam = "Altitude"
MethodName = "alt"
Counter = 1000

serialq = Q()
serialStateq = Q()
serialendq = Q()
sendq = Q()
SerialCommsIndicator = "Stop Serial"
serialStateq.put("Stop Serial")
SerialPort = ""

PointSymbol = '.'
MarkerSize = 5
PortSet = False
ThreadExit = False
ThreadStart = False
SerialException = False
PlotLoad = True


# print("Initial Parameters Set")
# ---------------------------------------------------------------------------------------
# Classes
# ---------------------------------------------------------------------------------------

class MainWindow(tk.Tk):
    # Creates the main window with filemenus and pages



    def __init__(self, *args, **kwargs):
        tk.Tk.__init__(self, *args, **kwargs)

        self.Ran = False

        self.configure(bg="white", highlightcolor="white", highlightbackground="white")
        self.wm_title("GCS (Version 1.0)")
        self.protocol('WM_DELETE_WINDOW', self.endprog)

        container = tk.Frame(self)
        container.pack(side="top", fill="both", expand=True)
        container.grid_rowconfigure(0, weight=1)
        container.grid_columnconfigure(0, weight=1)

        self.menubar = tk.Menu(container)

        filemenu = tk.Menu(self.menubar, tearoff=0)
        filemenu.add_radiobutton(label="Start Serial Comms", command=lambda: ControlSerial("start", self))
        filemenu.add_radiobutton(label="Pause Serial Comms", command=lambda: self.popup1("WARNING!",
                                                                                         "Any "
                                                                                         "incoming data "
                                                                                         "will be lost!",
                                                                                         "stop"))

        filemenu.add_radiobutton(label="End Serial Comms", command=lambda: self.popup1("WARNING!",
                                                                                       "Serial "
                                                                                       "Communication "
                                                                                       "Will be Terminated",
                                                                                       "end"))

        filemenu.add_separator()
        filemenu.add_command(label="Exit",
                             command=self.endprog)
        filemenu.configure(bg=BACKGROUND_MENU_COLOR, fg=TEXT_COLOR)
        self.menubar.add_cascade(label="File", menu=filemenu)
        # DisplayChoice
        #self.MainBackend(menubar)

        PlotControl = tk.Menu(self.menubar, tearoff=0)
        PlotControl.configure(bg=BACKGROUND_MENU_COLOR, fg=TEXT_COLOR)
        PlotControl.add_radiobutton(label="Resume Plot",
                                    command=lambda: LoadPlot('start'))
        PlotControl.add_radiobutton(label="Pause Plot",
                                    command=lambda: LoadPlot('pause'))
        self.menubar.add_cascade(label="Pause/Resume Plotting", menu=PlotControl)

        PortMenu = tk.Menu(self.menubar, tearoff=0)
        PortMenu.configure(bg=BACKGROUND_MENU_COLOR, fg=TEXT_COLOR)

        PortMenu.add_command(label="Enter Port",
                             command=lambda: self.PopPortDia("Enter Port (COMX):"))
        self.menubar.add_cascade(label="Port", menu=PortMenu)

        tk.Tk.config(self, menu=self.menubar)

        self.frames = {}

        for F in (StartPage, PageThree):
            frame = F(container, self)

            self.frames[F] = frame

            frame.grid(row=0, column=0, sticky="nsew")

        self.show_frame(PageThree)

    def show_frame(self, cont):
        # raises the given frame in frames
        frame = self.frames[cont]
        frame.tkraise()

    def endprog(self):
        # ends the entire program
        global ThreadExit
        ThreadExit = True
        self.destroy()
        self.quit()  # stops mainloop
        sys.exit(0)

    def PopPortDia(self, msg):
        # method for the port dialogue
        def leavemini():
            # method to destroy the popup
            pop.destroy()

        def get_set_parameter():
            # sets the serial port global variable and checks for port existence
            global SerialPort
            global PortSet
            SerialPort = Entry.get()
            print("you inputed:")
            print(SerialPort)

            if (SerialPort.startswith("COM")):
                SerialPort = SerialPort.upper()

            if (True):
                try:
                    testSerial = serial.Serial(port=SerialPort)
                    testSerial.close()
                    PortSet = True  # this may not be reseting when disconnection occurs
                    SerialThreadStart(SerialPort)
                    global SerialCommsIndicator
                    SerialCommsIndicator = "Stop Serial"
                    pop.destroy()
                except serial.SerialException:
                    self.popup("Nothing connected to Port, Try Again")

            else:
                self.popup("Not a Valid Port, Try Again")

        # GUI elements
        pop = tk.Toplevel(self)
        pop.grab_set()
        pop.resizable(width=False, height=False)
        pop.wm_title("!")
        label = ttk.Label(pop, text=msg, font=MEDIUM_FONT)
        label.pack(side="top", fill="x", pady=10, padx=5)

        Entry = ttk.Entry(pop)
        Entry.pack(side="top", fill='x', pady=10, padx=5)

        B1 = ttk.Button(pop, text="Establish Connection", command=lambda: get_set_parameter())
        B1.pack()
        pop.mainloop()

    def popup(self, msg):
        # generic popup box with message input option

        def leavemini():
            pop.destroy()

        # GUI elements
        pop = tk.Toplevel(self)
        pop.grab_set()
        pop.resizable(width=False, height=False)
        pop.wm_title("!")
        label = ttk.Label(pop, text=msg, font=MEDIUM_FONT)
        label.pack(side="top", fill="x", pady=10)
        B1 = ttk.Button(pop, text="Okay", command=leavemini)
        B1.pack()
        pop.mainloop()

    def popup1(self, msg, msg2, commandstr):
        # this popup is for displaying a warning before changing the Serial control global variable
        def leavemini():
            pop.destroy()

        pop = tk.Toplevel(self)
        pop.grab_set()

        pop.wm_title("!")
        label = ttk.Label(pop, text=msg, font=LARGE_FONT)
        label.pack(side="top", pady=10, expand=True)

        label2 = ttk.Label(pop, text=msg2, font=MEDIUM_FONT)
        label2.pack(side="top", pady=10, expand=True)
        B1 = ttk.Button(pop, text="Okay", command=lambda: ControlSerial(commandstr, pop))
        B1.pack()
        pop.resizable(width=False, height=False)
        pop.mainloop()

    def MainBackend(self):

        SerialEndSetParams()
        if (SerialCommsIndicator == "Start Serial" and ThreadStart == True):
            while (True):
                try:
                    serialData = serialq.get(0)
                    # serialq.task_done()
                except Queue.Empty:
                    # serialData = ""
                    break
                # print(serialData)
                fo = open(LogName, "a+")
                fo.write(serialData)
                fo.close()

        if (SerialCommsIndicator == "Start Serial" and ThreadStart == True):
            while (serialStateq.empty == False):
                serialStateq.get()
            serialStateq.put("Start Serial", 0)

        elif (SerialCommsIndicator == "End Serial" and ThreadStart == True):
            while (serialStateq.empty == False):
                serialStateq.get()
            serialStateq.put("End Serial", 0)

        if(PlotLoad or Counter):
            self.data = FileParse()

            TimeCol = 0
            NumLists = 0

            if(self.Ran == False):
                self.Ran = True
                DisplayChoice = tk.Menu(self.menubar, tearoff=1)
                DisplayChoice.configure(bg=BACKGROUND_MENU_COLOR, fg=TEXT_COLOR)

                # Find time column and number of lists and populate Display menu.

                for i,List in enumerate(self.data):
                    if(List[0]=="Time" or List[0]=="time"):
                        TimeCol = i

                    NumLists = i
                    print(str(TimeCol) + " " + str(NumLists))

                    DisplayChoice.add_radiobutton(label=self.data[i][0],
                                                  command=lambda: ChangeDisplay(self.data[i][0],self.data[i][0]))
                print(NumLists)
                DisplayChoice.add_radiobutton(label="Show All", command=lambda: ChangeDisplay("All", "all"))
                self.menubar.add_cascade(label="View", menu=DisplayChoice)

            # Configure timedates...
            timeListConfigured = []
            dateindex = 0
            for k, val in enumerate(self.data[TimeCol]):
                # print(repr(timeList[k]))
                try:
                    datetimeobject = datetime.strptime('2015 ' + str(self.data[TimeCol][k+1]), '%Y %I%M%S')
                    timeListConfigured.append(datetimeobject)
                    dateindex = k
                except:
                    pass
            print(len(timeListConfigured))
            timeDates = dates.date2num(timeListConfigured)

            if(GraphParam != "All"):
                a = plt.subplot2grid((6, 4), (0, 0), rowspan=6, colspan=4, axisbg=GRAPH_BG)
                for i, list in enumerate(self.data):
                     if(GraphParam==self.data[i][0]):
                         tempList = self.data[i][1:]
                         print(len(tempList))
                         print(len(timeDates))
                         ViewPlot(a, timeDates, tempList, dateindex,
                                  title=self.data[i][0],
                                  color=ALTITUDE_COLOR,
                                  marker=PointSymbol,
                                  markersize=MarkerSize)

            if(GraphParam == "All"):
                GraphObjs = []
                x = math.ceil(math.sqrt(NumLists))
                y = math.ceil((NumLists - x)/x) + 1
                j = 0
                k = 0
                for i, list in enumerate(self.data):
                    k = i - 1
                    if(k>y-1):
                        k = 0

                    GraphObjs.append(plt.subplot2grid((x, y), (j, k), rowspan=1, colspan=1, axisbg=GRAPH_BG))

                    k += 1

                for i, list in enumerate(self.data):
                    ViewPlot(GraphObjs[i], timeDates, self.data[i], dateindex,
                             title=self.data[i][0],
                             color=ALTITUDE_COLOR,
                             marker=PointSymbol,
                             markersize=MarkerSize)

            plt.tight_layout(pad=3)
            ob.frames[PageThree].canvas.show()




class PageThree(tk.Frame):
    # creates the main page, was called page three may change name later
    # contains all the GUI elements of the plot page
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)

        self.configure(bg=PADDING_COLOR, highlightcolor=PADDING_COLOR, highlightbackground=PADDING_COLOR)

        self.canvas = FigureCanvasTkAgg(f, self)
        self.canvas.get_tk_widget().grid(row=0, column=0, sticky='nsew', padx=5, pady=4, ipadx=0, ipady=0)
        self.canvas.get_tk_widget().configure(bg=BACKGROUND_COLOR, highlightcolor=PADDING_COLOR,
                                              highlightbackground=PADDING_COLOR, relief=tk.SUNKEN, bd=1)

        self.grid_rowconfigure(0, weight=1)
        self.grid_columnconfigure(0, weight=1)

        frame2 = tk.Frame(self)
        frame2.grid(row=1, column=0, columnspan=1, sticky='WE', padx=7, pady=5)
        frame2.configure(bg=BACKGROUND_COLOR, highlightcolor=BACKGROUND_COLOR, highlightbackground=BACKGROUND_COLOR,
                         relief=tk.SUNKEN, bd=1)
        # frame2.pack(side=tk.BOTTOM, fill=tk.BOTH, expand=1)

        frame3 = tk.Frame(self)
        frame3.grid(row=0, column=2, rowspan=2, sticky='NS', padx=3, pady=4)
        frame3.configure(bg=PADDING_COLOR, highlightcolor=PADDING_COLOR, highlightbackground=PADDING_COLOR)

        buttonsFrame = tk.Frame(self, relief=tk.SUNKEN, bd=1)
        buttonsFrame.configure(bg=BACKGROUND_COLOR, highlightcolor=BACKGROUND_COLOR,
                               highlightbackground=BACKGROUND_COLOR)
        buttonsFrame.grid(row=0, column=1, rowspan=2, sticky='NS', pady=5.2, padx=2)
        button1 = ttk.Button(buttonsFrame,
                             text="Force Release",
                             cursor='hand2',
                             command=lambda: SendPacket('r'))
        button1.grid(row=0, column=0, padx=2, pady=2, sticky='nsew')
        button2 = ttk.Button(buttonsFrame,
                             text="Capture",
                             cursor='hand2',
                             command=lambda: SendPacket('c'))
        button2.grid(row=0, column=1, padx=0, pady=2, sticky='nsew')
        button3 = ttk.Button(buttonsFrame,
                             text="Button 3",
                             cursor='hand2',
                             command=lambda: controller.show_frame(StartPage))
        button3.grid(row=1, column=0, padx=2, pady=0, sticky='nsew')
        button4 = ttk.Button(buttonsFrame,
                             text="Button 4",
                             cursor='hand2',
                             command=lambda: controller.show_frame(StartPage))
        button4.grid(row=1, column=1, padx=0, pady=0, sticky='nsew')

        rangelabel = tk.Label(buttonsFrame, text="Plot Range", font=SMALL_FONT)
        rangelabel.grid(row=2, column=0, columnspan=2, sticky='NS', padx=5, pady=5)
        rangelabel.configure(bg=BACKGROUND_COLOR, fg=TEXT_COLOR, highlightcolor=BACKGROUND_COLOR,
                             highlightbackground=BACKGROUND_COLOR)
        self.rangeentry1 = ttk.Entry(buttonsFrame, width=5)
        self.rangeentry1.grid(row=3, column=0, columnspan=2, sticky='NS', padx=5, pady=5)
        rangebutton = ttk.Button(buttonsFrame, text="Update Range", cursor='hand2', command=self.GetSetPlotRange)
        rangebutton.grid(row=4, column=0, columnspan=2, sticky='NS', padx=5, pady=5)
        # --------------------------------------------------------------------------------------------------------------

        mfFrame = tk.Frame(buttonsFrame)
        mfFrame.grid(row=5, column=0, columnspan=2, sticky='nsew', padx=5, pady=5)
        mfFrame.configure(bg=BACKGROUND_COLOR, highlightcolor=BACKGROUND_COLOR,
                          highlightbackground=BACKGROUND_COLOR)
        mfscrollbar = ttk.Scrollbar(mfFrame)
        mfscrollbar.grid(row=0, column=2, sticky='NS')
        self.marklistbox = tk.Listbox(mfFrame, selectmode=tk.SINGLE, yscrollcommand=mfscrollbar.set)
        self.marklistbox.grid(row=0, column=0, columnspan=2, sticky='nsew')
        mfscrollbar.config(command=self.marklistbox.yview)
        markerlist = ['point', 'pixel',
                      'circle', 'triangledown',
                      'triangleup', 'triangleleft',
                      'triangleright', 'octagon',
                      'square', 'pentagon',
                      'star', 'hexagon1',
                      'hexagon2', 'plus',
                      'X', 'diamond',
                      'thindiamond', 'vline',
                      'hline', 'None']
        for marker in markerlist:
            self.marklistbox.insert(tk.END, marker)
        mkbutton = ttk.Button(mfFrame, text='Set Marker', cursor='hand2', command=lambda: self.GetSetMarkerSymbol())
        mkbutton.grid(row=1, column=1, columnspan=1, sticky='NS', padx=5, pady=5)
        self.mksize = tk.Entry(mfFrame, width=5)
        self.mksize.grid(row=1, column=0, padx=5, pady=5)

        toolbar = NavigationToolbar2TkAgg(self.canvas, frame2)
        toolbar.update()
        toolbar.configure(bg=BACKGROUND_COLOR, highlightcolor=BACKGROUND_COLOR, highlightbackground=BACKGROUND_COLOR,
                          padx=1)

    def GetSetMarkerSymbol(self):
        # this method sets the symbol of the plot points
        global PointSymbol
        global MarkerSize
        Size = self.mksize.get()
        PointOption = self.marklistbox.curselection()
        print('Marker Size: ' + str(Size))
        print('Marker Selection: ' + str(PointOption))
        markers = ['.', ',',
                   'o', 'v',
                   '^', '<',
                   '>', '8',
                   's', 'p',
                   '*', 'h',
                   'H', '+',
                   'x', 'D',
                   'd', '|',
                   '_', 'None']
        try:
            PointOption = PointOption[0]
            PointSymbol = markers[PointOption]

        except:
            PointSymbol = PointSymbol
        try:
            if (float(Size) <= 0):
                return
            MarkerSize = float(Size)
            print(MarkerSize)
        except ValueError:
            pass
        UpdateCanvas()

    def GetSetPlotRange(self):
        # sets the global NumberOfPoints from the entry box
        # has some limiting if statements
        global NumberOfPoints
        string = self.rangeentry1.get()
        try:
            NumberOfPoints = int(string)
            if (NumberOfPoints == 0):
                global NumberOfPoints
                NumberOfPoints = 0
            elif (NumberOfPoints < 2):
                global NumberOfPoints
                NumberOfPoints = 2

        except ValueError:
            pass
        UpdateCanvas()


class StartPage(tk.Frame):
    # creates a blank start page may be used later for something
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        self.configure(bg="white", highlightcolor="white", highlightbackground="white")
        label = tk.Label(self, text="Start Page", font=LARGE_FONT)
        label.pack(pady=10, padx=10)

        button3 = ttk.Button(self, text="Visit Graph",
                             command=lambda: controller.show_frame(PageThree))
        button3.pack()


def animate(i):
    # this function runs every second and interupts the software loop.
    # basically the backend of the program

    timeprev = time.time()
    backend()
    print('t: ' + str(timeprev - time.time()))


def backend():
    SerialEndSetParams()
    if (SerialCommsIndicator == "Start Serial" and ThreadStart == True):
        while (True):
            try:
                serialData = serialq.get(0)
                # serialq.task_done()
            except Queue.Empty:
                # serialData = ""
                break
            # print(serialData)
            fo = open(LogName, "a+")
            fo.write(serialData)
            fo.close()

    if (SerialCommsIndicator == "Start Serial" and ThreadStart == True):
        while (serialStateq.empty == False):
            serialStateq.get()
        serialStateq.put("Start Serial", 0)

    elif (SerialCommsIndicator == "End Serial" and ThreadStart == True):
        while (serialStateq.empty == False):
            serialStateq.get()
        serialStateq.put("End Serial", 0)

    if (PlotLoad or Counter):
        # major if statement that opens log file and reads as well as writes to it
        # in addition updates plot from log file
        global Counter
        Counter = 0
        try:
            fo2 = open(LogName, "r")
        except:
            fo2 = open(LogName, "a+")
        getData = fo2.read()
        fo2.close()
        dataLine = getData.split('\n')
        datalnList = []
        altList = []
        pressList = []
        spdList = []
        tempList = []
        voltList = []
        gpsspdList = []
        timeList = []
        timeListConfigured = []
        dateindex = 0

        for eachline, string in enumerate(dataLine):
            datalnList = parse_serial(dataLine[eachline])
            # print(datalnList[1])
            if (len(datalnList) >= 14):
                altList.append(float(datalnList[2]))
                pressList.append(float(datalnList[3]))
                spdList.append(float(datalnList[4]))
                tempList.append(float(datalnList[5]))
                voltList.append(float(datalnList[6]))
                gpsspdList.append(float(datalnList[11]))
                timeList.append(datalnList[12])

        for k, val in enumerate(timeList):
            # print(repr(timeList[k]))
            datetimeobject = datetime.strptime('2015 ' + str(timeList[k]), '%Y %I:%M:%S')
            timeListConfigured.append(datetimeobject)
            dateindex = k

        timeDates = dates.date2num(timeListConfigured)
        if (GraphParam != "All"):
            a = plt.subplot2grid((6, 4), (0, 0), rowspan=6, colspan=4, axisbg=GRAPH_BG)
            if (GraphParam == "Altitude"):
                ViewPlot(a, timeDates, altList, dateindex, title="Altitude", color=ALTITUDE_COLOR, marker=PointSymbol,
                         markersize=MarkerSize)
            if (GraphParam == "Pressure"):
                ViewPlot(a, timeDates, pressList, dateindex, title="Pressure", color=PRESSURE_COLOR, marker=PointSymbol,
                         markersize=MarkerSize)
            if (GraphParam == "Speed"):
                ViewPlot(a, timeDates, spdList, dateindex, title="Speed", color=SPEED_COLOR, marker=PointSymbol,
                         markersize=MarkerSize)
            if (GraphParam == "Temperature"):
                ViewPlot(a, timeDates, tempList, dateindex, title="Temperature", color=TEMPERATURE_COLOR,
                         marker=PointSymbol, markersize=MarkerSize)
            if (GraphParam == "Voltage"):
                ViewPlot(a, timeDates, voltList, dateindex, title="Voltage", color=VOLTAGE_COLOR, marker=PointSymbol,
                         markersize=MarkerSize)
            if (GraphParam == "GPS Speed"):
                ViewPlot(a, timeDates, gpsspdList, dateindex, title="GPS Speed", color=GPSSPD_COLOR, marker=PointSymbol,
                         markersize=MarkerSize)

        if (GraphParam == "All"):
            a = plt.subplot2grid((4, 6), (0, 0), rowspan=2, colspan=2, axisbg=GRAPH_BG)
            ViewPlot(a, timeDates, altList, dateindex, title="Altitude", color=ALTITUDE_COLOR, marker=PointSymbol,
                     markersize=MarkerSize)
            p = plt.subplot2grid((4, 6), (0, 2), rowspan=2, colspan=2, axisbg=GRAPH_BG)
            ViewPlot(p, timeDates, pressList, dateindex, title="Pressure", color=PRESSURE_COLOR, marker=PointSymbol,
                     markersize=MarkerSize)
            s = plt.subplot2grid((4, 6), (0, 4), rowspan=2, colspan=2, axisbg=GRAPH_BG)
            ViewPlot(s, timeDates, spdList, dateindex, title="Speed", color=SPEED_COLOR, marker=PointSymbol,
                     markersize=MarkerSize)
            t = plt.subplot2grid((4, 6), (2, 0), rowspan=2, colspan=2, axisbg=GRAPH_BG)
            ViewPlot(t, timeDates, tempList, dateindex, title="Temperature", color=TEMPERATURE_COLOR,
                     marker=PointSymbol, markersize=MarkerSize)
            v = plt.subplot2grid((4, 6), (2, 2), rowspan=2, colspan=2, axisbg=GRAPH_BG)
            ViewPlot(v, timeDates, voltList, dateindex, title="Voltage", color=VOLTAGE_COLOR, marker=PointSymbol,
                     markersize=MarkerSize)
            g = plt.subplot2grid((4, 6), (2, 4), rowspan=2, colspan=2, axisbg=GRAPH_BG)
            ViewPlot(g, timeDates, gpsspdList, dateindex, title="GPS Speed", color=GPSSPD_COLOR, marker=PointSymbol,
                     markersize=MarkerSize)
        plt.tight_layout(pad=3)
        ob.frames[PageThree].canvas.show()


def ChangeDisplay(WhatDisp, WhatMeth):
    # method that changes the displayed plot on the graph frame
    global GraphParam
    global MethodName
    global Counter

    GraphParam = WhatDisp
    MethodName = WhatMeth
    Counter = 1000


def LoadPlot(run):
    # method that controls whether to plot to the gui or not.
    # useful when navigating on the plots since they update every second
    global PlotLoad

    if (run == "start"):
        PlotLoad = True

    elif (run == "pause"):
        PlotLoad = False


def ViewPlot(PltObj, Dates, Ylist, dateindex, xlabel="Time", title="Plot", timefmt="%I:%M:%S",
             linestyle='-', color="#00EE76", marker='.', markersize=5, alpha=.7,
             antialiased=True, solid_capstyle='round', solid_joinstyle='bevel'):
    # This method will plot a trace given the inputed lists and other visual parameters
    global NumberOfPoints
    PltObj.clear()
    Title = title
    PltObj.set_xlabel(xlabel)
    PltObj.set_title(Title, color=color)

    PltObj.xaxis.set_major_formatter(matplotlib.dates.DateFormatter(timefmt))
    PltObj.plot_date(Dates, Ylist,
                     linestyle=linestyle,
                     color=color,
                     marker=marker,
                     markersize=markersize,
                     alpha=alpha,
                     antialiased=antialiased,
                     solid_capstyle=solid_capstyle,
                     solid_joinstyle=solid_joinstyle)

    if ((dateindex - (NumberOfPoints - 1)) > 0 and (dateindex - (NumberOfPoints - 1)) < dateindex):
        plt.xlim(Dates[dateindex - (NumberOfPoints - 1)], Dates[dateindex])
    else:
        pass


def ControlSerial(run, pop):
    # method that sets the serialcommsindicator global
    global SerialCommsIndicator
    global ThreadStart

    if (ThreadStart == False or PortSet == False):
        ob.popup("                       "
                 "Port Not Set!\n\n"
                 "Please Enter a Serial Port "
                 "Before Starting Serial!")
        # SerialCommsIndicator = "End Serial"
        pop.destroy()
        return

    elif (run == "start"):
        SerialCommsIndicator = "Start Serial"

    elif (run == "stop"):
        SerialCommsIndicator = "Stop Serial"
        pop.destroy()
    elif (run == "end"):
        SerialCommsIndicator = "End Serial"
        pop.destroy()


def parse_serial(dataln):
    # parses log file into list of packet strings (each line is a packet string)
    previous = 0
    mylist = []
    for detected, val in enumerate(dataln):
        if dataln[detected] == ',':
            mylist.append(dataln[previous:detected])
            previous = detected + 1
        elif dataln[detected] == '\n':
            mylist.append(dataln[previous:detected - 1] + '\n')
    return mylist


def SerialEndSetParams():
    global ThreadStart
    global PortSet
    global SerialCommsIndicator
    if (serialendq.empty() == False):
        state = serialendq.get(0)
        if (state == 'end'):
            ThreadStart = False
            PortSet = False
            SerialCommsIndicator = 'Stop Serial'


def SerialComm(port):
    # this method sets up the serial port and starts serial comms.
    # it automatically ends during a serial exception (loss of connection)
    try:
        # print("thread start")
        serialObj = serial.Serial(port=port,
                                  baudrate=19200,
                                  parity=serial.PARITY_NONE,
                                  stopbits=serial.STOPBITS_ONE,
                                  bytesize=serial.EIGHTBITS,
                                  timeout=.05,
                                  rtscts=0,
                                  xonxoff=0,
                                  dsrdtr=0)
        serialObj.flushInput()
        serialObj.flushOutput()
        sleep(0.5)
        serialObj.flushInput()
        serialObj.flushOutput()
        # print("serial flushed!")
        # print("serial initialized")
        while (True):

            # print("looping")
            try:
                serState = serialStateq.get()
                print(serState)
            except serialStateq.empty:
                print(serState + 'emptyq')
                pass

            if (serState == "End Serial"):
                serialObj.close()
                print("Serial Comms Ended Successfully")
                serialendq.put('end')
                while (serialStateq.empty == False):
                    print('...')
                    serialStateq.get()
                print('Process End')
                return

            if (sendq.empty() == False):
                serialObj.write(sendq.get(0))

            try:
                if (serialObj.inWaiting() and serState == "Start Serial"):
                    serialData = serialObj.readline()
                    serialObj.flushInput()
                    if (serialData == "sending picture\n"):
                        img = open("Recieved.jpg", "w")
                        while (True):
                            dat = serialObj.readline()
                            if (dat == "end\n"):
                                break
                            else:
                                img.write(dat)

                    serialq.put(serialData, 0)

            except serial.SerialException:
                print("Serial Failed")
                return
    except serial.SerialException:
        print("Serial Failed")
        # global SerialException
        # SerialException = True
        return


def SerialThreadStart(port):
    # this method starts the serial comms on a separate thread so as not to interrupt the GUI loop
    t = multiprocessing.Process(target=SerialComm, args=(port,))
    t.daemon = True
    t.start()
    global ThreadStart
    ThreadStart = True


def SendPacket(packet):
    if (sendq.empty() == True):
        sendq.put(packet, 0)
    else:
        ob.popup("Still sending previous packet!")


def loop():
    # backend()
    # ob.frames[PageThree].canvas.draw()
    BackendThread()
    ob.after(1000, loop)


def BackendThread():
    #t2 = multiprocessing.Process(target=backend())
    t2 = multiprocessing.Process(target=ob.MainBackend())
    t2.daemon = True
    t2.start()
    t2.join()


def UpdateCanvas():
    global PlotLoad
    PlotLoadPrev = PlotLoad
    PlotLoad = True
    backend()
    print('Canvas Update')
    PlotLoad = PlotLoadPrev


def FileParse():  # will eventually allow any text file to be parsed and graphed
    try:
        fo2 = open(LogName, "r")
    except:
        fo2 = open(LogName, "a+")
    getData = fo2.read()
    fo2.close()

    lines = getData.split('\n')
    Headers = parse_serial(lines[0])
    data = []
    for i, header in enumerate(Headers):
        # poplates data list with lists
        data.append([])
    #print(data)

    for i, line in enumerate(lines):
        dataPoints = parse_serial(line)
        #print(dataPoints)
        # populates dataPoints with values in each line
        for j, list in enumerate(data):
            # populates sublists of each header with corresponding points of data.(2D array kinda)
            data[j-1].append(dataPoints[j-1])
            #print(data)
            # #######-------------------########## working here.
    #print(data)

    return data


# initialization and program start

if __name__ == '__main__':
    #FileParse()

    ob = MainWindow()
    ob.geometry("1280x720")

    ob.minsize(600, 400)
    ob.after(0, loop)
    # ani = animation.FuncAnimation(f, animate, interval=1000)

    if (PortSet == False):
        ob.popup("                       "
                 "Port Not Set!\n\nPlease "
                 "Enter a Serial Port Before "
                 "Starting Serial!")
    ob.mainloop()
