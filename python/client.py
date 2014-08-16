# Communication module to the DataCenter AC rotator.
# William Schoenell <william@iaa.es> - Aug 16, 2014

import argparse
import urllib
import csv

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
parser.add_argument("--url", help="Arduino URL address", default="http://192.168.1.131")
parser.add_argument("--temp_low", type=int, help="Set a new lower temperature treshold. In oC. Range (0-255).")
parser.add_argument("--temp_upp", type=int, help="Set a new upper temperature treshold. In oC. Range (0-255).")
parser.add_argument("--temp_int", type=int, help="Set a new temperature check interval. In seconds. Range (0-255).")
parser.add_argument("--rot_int", help="Set a new AC units rotation interval. hh:mm. Up to 255 hours.")
parser.add_argument("--rot_now", action="count", help="FORCE the AC units rotation now. WARNING: This can damage your units!")
parser.add_argument("--temp_now", action="count", help="FORCE the temperature check now.")
args = parser.parse_args()

cmd = False
for command in ('temp_low', 'temp_upp', 'temp_int'):
   if args.__getattribute__(command):
      cmd = True
      data = read_arduino(args.url+'/?cmd=%s&arg=%i' % (command, args.__getattribute__(command)))
      if int(data['cmd_status']) == 1 and int(float(data['cfg_'+command])) == args.__getattribute__(command):
         print (bcolors.OKGREEN+'        Command %s run successfully.'+bcolors.ENDC) % command
      else:
         print (bcolors.FAIL+'        Command %s run UN-successfully. Check for errors!!!'+bcolors.ENDC) % command

if args.rot_int:
   cmd = True
   h_n, m_n = args.rot_int.split(':')
   data = read_arduino(args.url+'/?cmd=rot_int&arg=%s' % args.rot_int)
   h_d, m_d = data['cfg_rot_int'].split(':')
   if int(h_n) == int(h_d) and int(m_n) == int(m_d) and data['cmd_status'] == 1:
      print bcolors.OKGREEN+'        Command rot_int run successfully.'+bcolors.ENDC
   else:
      print bcolors.FAIL+'        Command rot_int run UN-successfully. Check for errors!!!'+bcolors.ENDC

for command in ('rot_now', 'temp_now'):
   if args.__getattribute__(command) > 0 and raw_input(bcolors.WARNING+'This option shold be used with care.\nType YES if you really want to run this command: \n'+bcolors.ENDC) == 'YES':
     data = read_arduino(args.url+'/?cmd=%s&arg=0' % command)
     if int(data['cmd_status']) == 1: 
        print (bcolors.OKGREEN+'        Command %s run successfully.'+bcolors.ENDC) % command
     else:
        print (bcolors.FAIL+'        Command %s run UN-successfully. Check for errors!!!'+bcolors.ENDC) % command
   else:
     data = read_arduino(args.url)

if not cmd:
   data = read_arduino(args.url)

human_output(data)
