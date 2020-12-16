from flask import Flask, jsonify
from pathlib import Path
import os

app = Flask(__name__)

file='file.txt'

def SetState():
  if not os.path.exists(file):
    f_init = open(file, "w")
    f_init.write("False")
    f_init.close()
  f_open = open(file, "r")
  actual = f_open.read()
  f_write = open(file, "w")
  if actual == 'False':
    f_write.write("True")
  else:
    f_write.write("False")
  f_write.close()

def GetState():
  f_open = open(file, "r")
  return f_open.read()

@app.route('/change')
def change():
  SetState()
  return ('', 200)

@app.route('/state')
def def_state():
  return jsonify({"state": GetState()}), 200

if __name__ == '__main__':
  app.run( host='0.0.0.0' )

