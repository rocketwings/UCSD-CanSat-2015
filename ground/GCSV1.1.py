import Tkinter as tk
from datetime import datetime
import ttk
import serial
import matplotlib
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

serialq = Queue.Queue()
serialStateq = Queue.Queue()
SerialCommsIndicator = "Stop Serial"
serialStateq.put("Stop Serial")
SerialPort = ""

PortSet = False
ThreadExit = False
ThreadStart = False
SerialException = False
PlotLoad = True


# print("Initial Parameters Set")


class MainWindow(tk.Tk):
    def __init__(self, *args, **kwargs):
        tk.Tk.__init__(self, *args, **kwargs)

        self.configure(bg="white", highlightcolor="white", highlightbackground="white")

        self.wm_title("GCS (Version 1.0)")
        self.protocol('WM_DELETE_WINDOW', self.endprog)

        container = tk.Frame(self)
        container.pack(side="top", fill="both", expand=True)
        container.grid_rowconfigure(0, weight=1)
        container.grid_columnconfigure(0, weight=1)

        menubar = tk.Menu(container)

        filemenu = tk.Menu(menubar, tearoff=0)
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
        menubar.add_cascade(label="File", menu=filemenu)

        DisplayChoice = tk.Menu(menubar, tearoff=1)
        DisplayChoice.configure(bg=BACKGROUND_MENU_COLOR, fg=TEXT_COLOR)
        DisplayChoice.add_radiobutton(label="Altitude",
                                      command=lambda: ChangeDisplay("Altitude", "alt"))
        DisplayChoice.add_radiobutton(label="Pressure",
                                      command=lambda: ChangeDisplay("Pressure", "pressure"))
        DisplayChoice.add_radiobutton(label="Speed",
                                      command=lambda: ChangeDisplay("Speed", "speed"))
        DisplayChoice.add_radiobutton(label="Temperature",
                                      command=lambda: ChangeDisplay("Temperature", "Temp"))
        DisplayChoice.add_radiobutton(label="Voltage",
                                      command=lambda: ChangeDisplay("Voltage", "volt"))
        DisplayChoice.add_radiobutton(label="GPS Speed",
                                      command=lambda: ChangeDisplay("GPS Speed", "gpsspd"))
        DisplayChoice.add_radiobutton(label="Show All",
                                      command=lambda: ChangeDisplay("All", "all"))
        menubar.add_cascade(label="View", menu=DisplayChoice)

        PlotControl = tk.Menu(menubar, tearoff=0)
        PlotControl.configure(bg=BACKGROUND_MENU_COLOR, fg=TEXT_COLOR)
        PlotControl.add_radiobutton(label="Resume Plot",
                                    command=lambda: LoadPlot('start'))
        PlotControl.add_radiobutton(label="Pause Plot",
                                    command=lambda: LoadPlot('pause'))
        menubar.add_cascade(label="Pause/Resume Plotting", menu=PlotControl)

        PortMenu = tk.Menu(menubar, tearoff=0)
        PortMenu.configure(bg=BACKGROUND_MENU_COLOR, fg=TEXT_COLOR)

        PortMenu.add_command(label="Enter Port",
                             command=lambda: self.PopPortDia("Enter Port (COMX):"))
        menubar.add_cascade(label="Port", menu=PortMenu)

        tk.Tk.config(self, menu=menubar)

        self.frames = {}

        for F in (StartPage, PageThree):
            frame = F(container, self)

            self.frames[F] = frame

            frame.grid(row=0, column=0, sticky="nsew")

        self.show_frame(PageThree)

    def show_frame(self, cont):

        frame = self.frames[cont]
        frame.tkraise()

    def endprog(self):
        global ThreadExit
        ThreadExit = True
        self.destroy()
        self.quit()  # stops mainloop
        sys.exit(0)

    def PopPortDia(self, msg):

        def leavemini():
            pop.destroy()

        def EstablishSerial():
            global SerialPort
            SerialThreadStart(SerialPort)

        def get_set_parameter():
            global SerialPort
            global PortSet
            SerialPort = Entry.get()
            SerialPort = SerialPort.upper()
            if (SerialPort.startswith("COM")):
                try:
                    testSerial = serial.Serial(port=SerialPort)
                    testSerial.close()
                    PortSet = True
                    EstablishSerial()
                    pop.destroy()
                except serial.SerialException:
                    self.popup("Nothing connected to Port, Try Again")

            else:
                self.popup("Not a Valid Port, Try Again")

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

        def leavemini():
            pop.destroy()

        pop = tk.Toplevel(self)
        pop.grab_set()
        pop.resizable(width=False, height=False)
        pop.wm_title("!")
        label = ttk.Label(pop, text=msg, font=MEDIUM_FONT)
        label.pack(side="top", fill="x", pady=10)
        B1 = ttk.Button(pop, text="Okay", command=leavemini)
        B1.pack()
        pop.mainloop()

    def popup2(self, msg, popfather):

        def leavemini():
            pop.destroy()

        def get_set_parameter():
            global SerialPort
            global PortSet
            SerialPort = Entry.get()

            SerialPort.upper()

            if (SerialPort.startswith("COM")):
                try:
                    testSerial = serial.Serial(port=SerialPort)
                    testSerial.close()
                    PortSet = True
                    pop.destroy()
                except serial.SerialException:
                    self.popup("Nothing connected to Port, Try Again")

            else:
                self.popup("Not a Valid Port, Try Again")

        popfather.destroy()
        pop = tk.Toplevel(self)
        pop.grab_set()
        pop.resizable(width=False, height=False)
        pop.wm_title("!")
        label = ttk.Label(pop, text=msg, font=MEDIUM_FONT)
        label.pack(side="top", fill="x", pady=10, padx=5)

        Entry = ttk.Entry(pop)
        Entry.pack(side="top", fill='x', pady=10, padx=5)

        B1 = ttk.Button(pop, text="Okay", command=lambda: get_set_parameter())
        B1.pack()
        pop.mainloop()

    def popup2popup(self, msg1, msg2):

        def leavemini():
            pop.destroy()

        pop = tk.Toplevel(self)
        pop.grab_set()
        pop.resizable(width=False, height=False)
        pop.wm_title("!")
        label = ttk.Label(pop, text=msg1, font=MEDIUM_FONT)
        label.pack(side="top", fill="x", pady=10)
        B1 = ttk.Button(pop, text="Okay", command=lambda: self.popup2(msg2, pop))
        B1.pack()
        pop.mainloop()

    def popup1(self, msg, msg2, commandstr):

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


class StartPage(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        self.configure(bg="white", highlightcolor="white", highlightbackground="white")
        label = tk.Label(self, text="Start Page", font=LARGE_FONT)
        label.pack(pady=10, padx=10)

        button3 = ttk.Button(self, text="Visit Graph",
                             command=lambda: controller.show_frame(PageThree))
        button3.pack()


class PageThree(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)

        self.configure(bg=PADDING_COLOR, highlightcolor=PADDING_COLOR, highlightbackground=PADDING_COLOR)

        canvas = FigureCanvasTkAgg(f, self)
        canvas.show()
        canvas.get_tk_widget().grid(row=0, column=0, sticky='nsew', padx=5, pady=4, ipadx=0, ipady=0)
        canvas.get_tk_widget().configure(bg=BACKGROUND_COLOR, highlightcolor=PADDING_COLOR,
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
                             text="Button 1",
                             cursor='hand2',
                             command=lambda: controller.show_frame(StartPage))
        button1.grid(row=0, column=0, padx=2, pady=2, sticky='nsew')
        button2 = ttk.Button(buttonsFrame,
                             text="Button 2",
                             cursor='hand2',
                             command=lambda: controller.show_frame(StartPage))
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
        rangelabel.grid(row=2,column=0,columnspan=2,sticky='NS', padx=5, pady=5)
        rangelabel.configure(bg=BACKGROUND_COLOR, fg=TEXT_COLOR, highlightcolor=BACKGROUND_COLOR, highlightbackground=BACKGROUND_COLOR)
        self.rangeentry1 = ttk.Entry(buttonsFrame, width=5)
        self.rangeentry1.grid(row=3,column=0,columnspan=2,sticky='NS', padx=5, pady=5)
        rangebutton = ttk.Button(buttonsFrame,text="Update Range", cursor='hand2', command=self.GetSetPlotRange)
        rangebutton.grid(row=4,column=0,columnspan=2,sticky='NS', padx=5, pady=5)

        toolbar = NavigationToolbar2TkAgg(canvas, frame2)
        toolbar.update()
        toolbar.configure(bg=BACKGROUND_COLOR, highlightcolor=BACKGROUND_COLOR, highlightbackground=BACKGROUND_COLOR,
                          padx=1)

    def GetSetPlotRange(self):
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


def animate(i):
    if (SerialCommsIndicator == "Start Serial" and ThreadStart == True):
        while (True):
            try:
                serialData = serialq.get(0)
                serialq.task_done()
            except Queue.Empty:
                # serialData = ""
                break
            # print(serialData)
            fo = open(LogName, "a")
            fo.write(serialData)
            fo.close()

    if (SerialCommsIndicator == "Start Serial" and ThreadStart == True):
        with serialStateq.mutex:
            serialStateq.queue.clear()
            # print("clearstart")
        serialStateq.put("Start Serial", 0)
        # print("starting serial")

    elif (SerialCommsIndicator == "End Serial" and ThreadStart == True):
        with serialStateq.mutex:
            serialStateq.queue.clear()
            # print("clearstart")
        serialStateq.put("End Serial", 0)
        global ThreadStart
        ThreadStart = False
        # print("stopping serial")

    elif (ThreadStart == True):
        with serialStateq.mutex:
            serialStateq.queue.clear()
            # print("clearstart")
        serialStateq.put("Stop Serial", 0)
        # print("stopping serial")

    if (PlotLoad or Counter):
        global Counter
        Counter = 0
        fo2 = open(LogName, "r")
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

        if (GraphParam == "Altitude"):
            a = plt.subplot2grid((6, 4), (0, 0), rowspan=6, colspan=4, axisbg=GRAPH_BG)
            a.clear()
            title = "Altitude Plot"
            a.set_xlabel('Time')
            a.set_title(title, color=ALTITUDE_COLOR)

            a.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%I:%M:%S'))
            a.plot_date(timeDates, altList,
                        linestyle='-',
                        color=ALTITUDE_COLOR,
                        marker='.',
                        alpha=.7,
                        antialiased=True,
                        solid_capstyle='round',
                        solid_joinstyle='bevel')

            if ((dateindex - (NumberOfPoints - 1)) > 0 and (dateindex - (NumberOfPoints - 1)) < dateindex):
                plt.xlim(timeDates[dateindex - (NumberOfPoints - 1)], timeDates[dateindex])
            else:
                pass

        if (GraphParam == "Pressure"):
            a = plt.subplot2grid((6, 4), (0, 0), rowspan=6, colspan=4, axisbg=GRAPH_BG)
            a.clear()
            title = "Pressure Plot"
            a.set_xlabel('Time')
            a.set_title(title, color=PRESSURE_COLOR)
            a.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%I:%M:%S'))
            a.plot_date(timeDates, pressList,
                        linestyle='-',
                        color=PRESSURE_COLOR,
                        marker='.',
                        alpha=.7,
                        antialiased=True,
                        solid_capstyle='round',
                        solid_joinstyle='bevel')
            if ((dateindex - (NumberOfPoints - 1)) > 0 and (dateindex - (NumberOfPoints - 1)) < dateindex):
                plt.xlim(timeDates[dateindex - (NumberOfPoints - 1)], timeDates[dateindex])
            else:
                pass

        if (GraphParam == "Speed"):
            a = plt.subplot2grid((6, 4), (0, 0), rowspan=6, colspan=4, axisbg=GRAPH_BG)
            a.clear()
            title = "Speed Plot"
            a.set_xlabel('Time')
            a.set_title(title, color=SPEED_COLOR)
            a.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%I:%M:%S'))
            a.plot_date(timeDates, spdList,
                        linestyle='-',
                        color=SPEED_COLOR,
                        marker='.',
                        alpha=.7,
                        antialiased=True,
                        solid_capstyle='round',
                        solid_joinstyle='bevel')
            if ((dateindex - (NumberOfPoints - 1)) > 0 and (dateindex - (NumberOfPoints - 1)) < dateindex):
                plt.xlim(timeDates[dateindex - (NumberOfPoints - 1)], timeDates[dateindex])
            else:
                pass

        if (GraphParam == "Temperature"):
            a = plt.subplot2grid((6, 4), (0, 0), rowspan=6, colspan=4, axisbg=GRAPH_BG)
            a.clear()
            title = "Temperature Plot"
            a.set_xlabel('Time')
            a.set_title(title, color=TEMPERATURE_COLOR)
            a.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%I:%M:%S'))
            a.plot_date(timeDates, tempList,
                        linestyle='-',
                        color=TEMPERATURE_COLOR,
                        marker='.',
                        alpha=.7,
                        antialiased=True,
                        solid_capstyle='round',
                        solid_joinstyle='bevel')
            if ((dateindex - (NumberOfPoints - 1)) > 0 and (dateindex - (NumberOfPoints - 1)) < dateindex):
                plt.xlim(timeDates[dateindex - (NumberOfPoints - 1)], timeDates[dateindex])
            else:
                pass

        if (GraphParam == "Voltage"):
            a = plt.subplot2grid((6, 4), (0, 0), rowspan=6, colspan=4, axisbg=GRAPH_BG)
            a.clear()
            title = "Voltage Plot"
            a.set_xlabel('Time')
            a.set_title(title, color=VOLTAGE_COLOR)
            a.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%I:%M:%S'))
            a.plot_date(timeDates, voltList,
                        linestyle='-',
                        color=VOLTAGE_COLOR,
                        marker='.',
                        alpha=.7,
                        antialiased=True,
                        solid_capstyle='round',
                        solid_joinstyle='bevel')
            if ((dateindex - (NumberOfPoints - 1)) > 0 and (dateindex - (NumberOfPoints - 1)) < dateindex):
                plt.xlim(timeDates[dateindex - (NumberOfPoints - 1)], timeDates[dateindex])
            else:
                pass

        if (GraphParam == "GPS Speed"):
            a = plt.subplot2grid((6, 4), (0, 0), rowspan=6, colspan=4, axisbg=GRAPH_BG)
            a.clear()
            title = "GPS Speed Plot"
            a.set_xlabel('Time')
            a.set_title(title, color=GPSSPD_COLOR)
            a.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%I:%M:%S'))
            a.plot_date(timeDates, gpsspdList,
                        linestyle='-',
                        color=GPSSPD_COLOR,
                        marker='.',
                        alpha=.7,
                        antialiased=True,
                        solid_capstyle='round',
                        solid_joinstyle='bevel')
            if ((dateindex - (NumberOfPoints - 1)) > 0 and (dateindex - (NumberOfPoints - 1)) < dateindex):
                plt.xlim(timeDates[dateindex - (NumberOfPoints - 1)], timeDates[dateindex])
            else:
                pass

        if (GraphParam == "All"):
            a = plt.subplot2grid((4, 6), (0, 0), rowspan=2, colspan=2, axisbg=GRAPH_BG)
            a.clear()
            a.set_xlabel('Time')
            a.set_title("Altitude", color=ALTITUDE_COLOR)
            a.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%I:%M:%S'))
            a.tick_params(axis='both', which='major', labelsize=8)
            a.plot_date(timeDates, altList,
                        linestyle='-',
                        color=ALTITUDE_COLOR,
                        marker='.',
                        alpha=.7,
                        antialiased=True,
                        solid_capstyle='round',
                        solid_joinstyle='bevel')
            if ((dateindex - (NumberOfPoints - 1)) > 0 and (dateindex - (NumberOfPoints - 1)) < dateindex):
                plt.xlim(timeDates[dateindex - (NumberOfPoints - 1)], timeDates[dateindex])
            else:
                pass

            p = plt.subplot2grid((4, 6), (0, 2), rowspan=2, colspan=2, axisbg=GRAPH_BG)
            p.clear()
            p.set_xlabel('Time')
            p.set_title("Pressure", color=PRESSURE_COLOR)
            p.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%I:%M:%S'))
            p.tick_params(axis='both', which='major', labelsize=8)
            p.plot_date(timeDates, pressList,
                        linestyle='-',
                        color=PRESSURE_COLOR,
                        marker='.',
                        alpha=.7,
                        antialiased=True,
                        solid_capstyle='round',
                        solid_joinstyle='bevel')
            if ((dateindex - (NumberOfPoints - 1)) > 0 and (dateindex - (NumberOfPoints - 1)) < dateindex):
                plt.xlim(timeDates[dateindex - (NumberOfPoints - 1)], timeDates[dateindex])
            else:
                pass

            s = plt.subplot2grid((4, 6), (0, 4), rowspan=2, colspan=2, axisbg=GRAPH_BG)
            s.clear()
            s.set_xlabel('Time')
            s.set_title("Speed", color=SPEED_COLOR)
            s.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%I:%M:%S'))
            s.tick_params(axis='both', which='major', labelsize=8)
            s.plot_date(timeDates, spdList,
                        linestyle='-',
                        color=SPEED_COLOR,
                        marker='.',
                        alpha=.7,
                        antialiased=True,
                        solid_capstyle='round',
                        solid_joinstyle='bevel')
            if ((dateindex - (NumberOfPoints - 1)) > 0 and (dateindex - (NumberOfPoints - 1)) < dateindex):
                plt.xlim(timeDates[dateindex - (NumberOfPoints - 1)], timeDates[dateindex])
            else:
                pass

            t = plt.subplot2grid((4, 6), (2, 0), rowspan=2, colspan=2, axisbg=GRAPH_BG)
            t.clear()
            t.set_xlabel('Time')
            t.set_title("Temperature", color=TEMPERATURE_COLOR)
            t.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%I:%M:%S'))
            t.tick_params(axis='both', which='major', labelsize=8)
            t.plot_date(timeDates, tempList,
                        linestyle='-',
                        color=TEMPERATURE_COLOR,
                        marker='.',
                        alpha=.7,
                        antialiased=True,
                        solid_capstyle='round',
                        solid_joinstyle='bevel')
            if ((dateindex - (NumberOfPoints - 1)) > 0 and (dateindex - (NumberOfPoints - 1)) < dateindex):
                plt.xlim(timeDates[dateindex - (NumberOfPoints - 1)], timeDates[dateindex])
            else:
                pass

            v = plt.subplot2grid((4, 6), (2, 2), rowspan=2, colspan=2, axisbg=GRAPH_BG)
            v.clear()
            v.set_xlabel('Time')
            v.set_title("Voltage", color=VOLTAGE_COLOR)
            v.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%I:%M:%S'))
            v.tick_params(axis='both', which='major', labelsize=8)
            v.plot_date(timeDates, voltList,
                        linestyle='-',
                        color=VOLTAGE_COLOR,
                        marker='.',
                        alpha=.7,
                        antialiased=True,
                        solid_capstyle='round',
                        solid_joinstyle='bevel')
            if ((dateindex - (NumberOfPoints - 1)) > 0 and (dateindex - (NumberOfPoints - 1)) < dateindex):
                plt.xlim(timeDates[dateindex - (NumberOfPoints - 1)], timeDates[dateindex])
            else:
                pass
            g = plt.subplot2grid((4, 6), (2, 4), rowspan=2, colspan=2, axisbg=GRAPH_BG)
            g.clear()
            g.set_xlabel('Time')
            g.set_title("GPS Speed", color=GPSSPD_COLOR)
            g.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%I:%M:%S'))
            g.tick_params(axis='both', which='major', labelsize=8)
            g.plot_date(timeDates, gpsspdList,
                        linestyle='-',
                        color=GPSSPD_COLOR,
                        marker='.',
                        alpha=.7,
                        antialiased=True,
                        solid_capstyle='round',
                        solid_joinstyle='bevel')
            if ((dateindex - (NumberOfPoints - 1)) > 0 and (dateindex - (NumberOfPoints - 1)) < dateindex):
                plt.xlim(timeDates[dateindex - (NumberOfPoints - 1)], timeDates[dateindex])
            else:
                pass

        plt.tight_layout(pad=3)


def ChangeDisplay(WhatDisp, WhatMeth):
    global GraphParam
    global MethodName
    global Counter

    GraphParam = WhatDisp
    MethodName = WhatMeth
    Counter = 1000


def LoadPlot(run):
    global PlotLoad

    if (run == "start"):
        PlotLoad = True

    elif (run == "pause"):
        PlotLoad = False


def ControlSerial(run, pop):
    global SerialCommsIndicator
    global ThreadStart

    if(ThreadStart == False):
        ob.popup("                       "
                 "Port Not Set!\n\n"
                 "Please Enter a Serial Port "
                 "Before Starting Serial!")
        pop.destroy()
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
    previous = 0
    mylist = []
    for detected, val in enumerate(dataln):
        if dataln[detected] == ',':
            mylist.append(dataln[previous:detected])
            previous = detected + 1
        elif dataln[detected] == '\n':
            mylist.append(dataln[previous:detected - 1] + '\n')
    return mylist


def SerialComm(port, dataq, statusq):
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

            serState = statusq.get()
            statusq.put(serState)

            if (serState == "End Serial"):
                serialObj.close()
                print("Serial Comms Ended Successfully")
                return

            # print(serState)
            # print("loopstart")
            # if (serState == "Start Serial"):
            try:

                if (serialObj.inWaiting() and serState == "Start Serial"):
                    serialData = serialObj.readline()
                    serialObj.flushInput()
                    dataq.put(serialData, 0)
                    print(serState)
                    # dataq.join()
                    # print(serialData)

                    # print("loopend")
            except serial.SerialException:
                print("Serial Failed")
                # tkMessageBox.showerror("Warning!","Serial Failed, Restart Program!")
                # sleep(1)
                return
    except serial.SerialException:
        print("Serial Failed")
        # global SerialException
        # SerialException = True
        return


def SerialThreadStart(port):
    t = threading.Thread(target=SerialComm, args=(port, serialq, serialStateq))
    t.daemon = True
    t.start()
    global ThreadStart
    ThreadStart = True




ob = MainWindow()
ob.geometry("1280x720")
ani = animation.FuncAnimation(f, animate, interval=1000)
if (PortSet == False):
    ob.popup("                       "
             "Port Not Set!\n\nPlease "
             "Enter a Serial Port Before "
             "Starting Serial!")
ob.mainloop()
