# Communication module to the DataCenter AC rotator.
# William Schoenell <william@iaa.es> - Aug 16, 2014

import argparse
import urllib
import csv
import sys
import time
import datetime

import numpy as np

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'


def read_arduino(url):
    ino_site = urllib.urlopen(url)
    reader = csv.reader(ino_site)
    keys = reader.next()
    values = reader.next()
    data = dict(zip(keys, values))
    data['url'] = url

    return data


def human_output(data):
    print('\n\
    	Arduino URL: %(url)s \n\
	=========================== CONFIGURATION =========================== \n\
	temp_low: %(cfg_temp_low)s oC\t - Lower temperature treshold - (0-255)  \n\
	temp_upp: %(cfg_temp_upp)s oC\t - Upper temperature treshold - (0-255)  \n\
	temp_interval: %(cfg_temp_int)s s\t - Temperature check interval - (0-255)  \n\
	rot_interval: %(cfg_rot_int)s h:m\t - AC units rotation interval - Up to 255h  \n\n\
	=============================== STATUS ============================== \n\
	act_unit: %(act_unit)s \t - Main AC unit now \n\
	slaves_on: %(slaves_on)s \t - 1 if slaves are turned on in an event of overheating \n\
	temp: %(temp)s oC\t - Ambient temperature now \n\
	hum: %(hum)s %%\t - Ambient humidity now \n ' % data)


parser = argparse.ArgumentParser()
parser.add_argument("--url", help="Arduino URL address", default="http://192.168.1.129")
parser.add_argument("--temp_low", type=int, help="Set a new lower temperature treshold. In oC. Range (0-255).")
parser.add_argument("--temp_upp", type=int, help="Set a new upper temperature treshold. In oC. Range (0-255).")
parser.add_argument("--temp_int", type=int, help="Set a new temperature check interval. In seconds. Range (0-255).")
parser.add_argument("--rot_int", help="Set a new AC units rotation interval. hh:mm. Up to 255 hours.")
parser.add_argument("--rot_now", action="count",
                    help="FORCE the AC units rotation now. WARNING: This can damage your units!")
parser.add_argument("--temp_now", action="count", help="FORCE the temperature check now.")
parser.add_argument("--daemon", action="count", help="Start in daemon mode, feeding data to plot.ly.")
parser.add_argument("--new", action="count", help="When with daemon, cleans the plot.ly plot before start.")
args = parser.parse_args()

cmd = False
for command in ('temp_low', 'temp_upp', 'temp_int'):
    if args.__getattribute__(command):
        cmd = True
        data = read_arduino(args.url + '/?cmd=%s&arg=%i' % (command, args.__getattribute__(command)))
        if int(data['cmd_status']) == 1 and int(float(data['cfg_' + command])) == args.__getattribute__(command):
            print (bcolors.OKGREEN + '        Command %s run successfully.' + bcolors.ENDC) % command
        else:
            print (
                  bcolors.FAIL + '        Command %s run UN-successfully. Check for errors!!!' + bcolors.ENDC) % command

if args.rot_int:
    cmd = True
    h_n, m_n = args.rot_int.split(':')
    data = read_arduino(args.url + '/?cmd=rot_int&arg=%s' % args.rot_int)
    h_d, m_d = data['cfg_rot_int'].split(':')
    if int(h_n) == int(h_d) and int(m_n) == int(m_d) and data['cmd_status'] == 1:
        print bcolors.OKGREEN + '        Command rot_int run successfully.' + bcolors.ENDC
    else:
        print bcolors.FAIL + '        Command rot_int run UN-successfully. Check for errors!!!' + bcolors.ENDC

for command in ('rot_now', 'temp_now'):
    if args.__getattribute__(command) > 0:
        cmd = True
        if raw_input(
                                bcolors.WARNING + 'This option shold be used with care.\nType YES if you really want to run this command: \n' + bcolors.ENDC) == 'YES':
            data = read_arduino(args.url + '/?cmd=%s&arg=0' % command)
            if int(data['cmd_status']) == 1:
                print (bcolors.OKGREEN + '        Command %s run successfully.' + bcolors.ENDC) % command
            else:
                print (
                      bcolors.FAIL + '        Command %s run UN-successfully. Check for errors!!!' + bcolors.ENDC) % command
        else:
            data = read_arduino(args.url)

if args.daemon:
    try:
        import plotly.plotly as py
        import plotly.tools as tls
        from plotly.graph_objs import Scatter, Data, Figure, YAxis, Layout, Font
    except ImportError:
        print 'Could not import plotly python module.'
        sys.exit(2)
    layout = Layout(
        title='Data Center enviroment data',
        yaxis=YAxis(
            title='Temperature (Celsius) / Humidity (%)',
            range=[10, 50]
        ),
        yaxis2=YAxis(
            title='ON/OFF',
            titlefont=Font(
                color='rgb(148, 103, 189)'
            ),
            tickfont=Font(
                color='rgb(148, 103, 189)'
            ),
            overlaying='y',
            side='right',
            range=[-.5, 1.5]
        )
    )
    trace1 = Scatter(x=[], y=[], name='temperature', stream=dict(token='aamkhlzl44', maxpoints=1440)) #, xaxis='x1')
    trace2 = Scatter(x=[], y=[], name='humidity', stream=dict(token='044mpl7nqo', maxpoints=1440)) #, xaxis='x2')
    trace3 = Scatter(x=[], y=[], name='Active AC', stream=dict(token='lsdi9172dd', maxpoints=1440), yaxis='y2')
    trace4 = Scatter(x=[], y=[], name='Slaves Active?', stream=dict(token='m4of2pjlx3', maxpoints=1440), yaxis='y2')
    fig = Figure(data=[trace1, trace2, trace3, trace4], layout=layout)
    if args.new > 0:
        py.plot(fig, filename='DataCenterACRotation')
    else:
        py.plot(fig, filename='DataCenterACRotation', fileopt='extend')
    s1 = py.Stream('aamkhlzl44')
    s2 = py.Stream('044mpl7nqo')
    s3 = py.Stream('lsdi9172dd')
    s4 = py.Stream('m4of2pjlx3')
    s1.open()
    s2.open()
    s3.open()
    s4.open()
    while True:
        data = read_arduino(args.url)
        if np.float(data['temp']) < 100:
            print '[%s] Writing to plot.ly server...' % datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            s1.write(dict(x=datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'), y=np.float(data['temp'])))
            s2.write(dict(x=datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'), y=np.float(data['hum'])))
            s3.write(dict(x=datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'), y=np.float(data['act_unit'])))
            s4.write(dict(x=datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'), y=np.float(data['slaves_on'])))
        else:
            print 'Skipping due to an incorrect temperature value...'
        time.sleep(60)
    s1.close()
    s2.close()
    s3.close()
    s4.close()


if not cmd:
    data = read_arduino(args.url)

human_output(data)
