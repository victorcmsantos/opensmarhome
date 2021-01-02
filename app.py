from flask import Flask, jsonify, request, render_template
import os
from flask_jwt_extended import JWTManager, jwt_required, create_access_token, get_jwt_identity
from werkzeug.security import generate_password_hash, check_password_hash
from flask_sqlalchemy import SQLAlchemy
import uuid
import json
from sqlalchemy import or_

basedir = os.path.abspath(os.path.dirname(__file__))

def uuid_gen():
  return str(uuid.uuid4().node)

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///' + os.path.join(basedir, 'database.sqlite')
app.config['SECRET_KEY'] = "you-will-never-guess"

db = SQLAlchemy(app)

app.config['JWT_SECRET_KEY'] = 'super_super-secret'
jwt = JWTManager(app)

data_file = '/opt/osh'

def file(userid_var, deviceid_var):
  if not os.path.exists(data_file):
    os.makedirs(data_file)
  a = os.path.join(data_file, userid_var)
  if not os.path.exists(a):
    os.makedirs(a)
  b = os.path.join(a, deviceid_var)
  if not os.path.exists(b):
    os.makedirs(b)
  c = os.path.join(b, 'status-file.txt')
  if not os.path.exists(c):
    f_init = open(c , "w")
    f_init.write("False")
    f_init.close()
  return c

def watts_file(userid_var, deviceid_var, value):
  if not os.path.exists(data_file):
    os.makedirs(data_file)
  a = os.path.join(data_file, userid_var)
  if not os.path.exists(a):
    os.makedirs(a)
  b = os.path.join(a, deviceid_var)
  if not os.path.exists(b):
    os.makedirs(b)
  c = os.path.join(b, 'watts')
  f_init = open(c , "w")
  f_init.write(value)
  f_init.close()

class User(db.Model):
  __tablename__ = 'users'
  id = db.Column(db.String(50), unique=True, index=True, nullable=False, default=uuid_gen, primary_key=True )
  #id = db.Column(db.Integer, primary_key=True)
  nickname = db.Column(db.String(64), index=True, unique=True,nullable=False)
  email = db.Column(db.String(120), index=True, unique=True, nullable=False)
  password_hash = db.Column(db.String(128))
  def __init__(self, nickname, email):
    self.nickname = nickname
    self.email = email
  def __repr__(self):
    return '<User {}>'.format(self.nickname)
  def set_password(self, password):
    self.password_hash = generate_password_hash(password)
  def check_password(self, password):
    return check_password_hash(self.password_hash, password)
  def all_info(self):
    return {'id': self.id,'email': self.email, 'nickname': self.nickname }
    
db.create_all()

def registerNewUser(nickname, password, email):
  status={}
  if not User.query.filter_by(email=email).first():
    u = User(nickname=nickname, email=email)
    u.set_password(password)
    db.session.add(u)
    db.session.commit()
    status[nickname]= "created"
  else:
    status[nickname]= "alread exists"

registerNewUser('victor', '12', 'victor@example.com')
registerNewUser('iorio', '12', 'iorio@example.com')

def getUserBy( email, userid ):
  if User.query.filter_by(email=email).first().id == userid:
    return True
  else:
    return False

def SetState(userid_var, deviceid_var):
  actual =  GetState(userid_var, deviceid_var)
  f_write = open(file(userid_var, deviceid_var), "w")
  if actual == 'False':
    f_write.write("True")
  else:
    f_write.write("False")
  f_write.close()

def GetState(userid_var, deviceid_var):
  f_open = open(file(userid_var, deviceid_var), "r")
  return f_open.read()

def getalldevices(userid_var):
  if not os.path.exists(data_file):
    os.makedirs(data_file)
  a = os.path.join(data_file, userid_var)
  if not os.path.exists(a):
    os.makedirs(a)
  ddict = {}
  ddict["metric_url"] = '%s/metrics/' % (userid_var)
  ddict["devices"] = {}
  for i in os.listdir( a ):
    ddict["devices"][i] = {}
    ddict["devices"][i]["state_url"] = '%s/%s/state/' % (userid_var, i)
    ddict["devices"][i]["change_url"] = '%s/%s/change/' % (userid_var, i)
  return(ddict)

def metrics(userid_var):
  arr=[]
  if not os.path.exists(data_file):
    os.makedirs(data_file)
  a = os.path.join(data_file, userid_var)
  if not os.path.exists(a):
    os.makedirs(a)
  for i in os.listdir( a ):
    c = os.path.join(a, i)
    d = os.path.join(c, 'watts')
    if os.path.exists(d):
      f = open(d, "r")
      arr.append('Watts{device_id="%s"} %s' % (i, f.read() ))
      os.remove(d)
  return arr

#############################################################
@app.route('/login', methods=['POST','GET'])
def login():
  if not request.is_json:
    return jsonify({"msg": "Missing JSON in request"}), 400
  email_f_post = request.json.get('email', None)
  password_f_post = request.json.get('password', None)
  if not email_f_post:
    return jsonify({"msg": "Missing email parameter"}), 400
  if not password_f_post:
    return jsonify({"msg": "Missing password parameter"}), 400
  user = User.query.filter_by(email=email_f_post).first()
  if user is None or not user.check_password(password_f_post):
    return jsonify({"msg": "Bad email or password"}), 401
  access_token = create_access_token(identity=email_f_post, expires_delta=False)
  userallinfo = user.all_info()
  userallinfo['token'] = access_token
  return jsonify( userallinfo ), 200

@app.route('/<path:userid_var>/')
@jwt_required
def devices(userid_var):
  if not getUserBy( get_jwt_identity(), userid_var ):
    return jsonify({"msg": "not authorized"}), 401
  return jsonify( getalldevices(userid_var) ), 200

@app.route('/<path:userid_var>/metrics/')
@jwt_required
def metrics_app(userid_var):
  if not getUserBy( get_jwt_identity(), userid_var ):
    return jsonify({"msg": "not authorized"}), 401
  return render_template('template', con=metrics(userid_var))

@app.route('/<path:userid_var>/<path:deviceid_var>/watts/', methods=['POST'])
@jwt_required
def def_watts(userid_var, deviceid_var):
  if not getUserBy( get_jwt_identity(), userid_var ):
    return jsonify({"msg": "not authorized"}), 401
  if not request.is_json:
    return jsonify({"msg": "Missing JSON in request"}), 400
  watts_f_post = request.json.get('watts', None)
  if not watts_f_post:
    return jsonify({"msg": "Missing watts parameter"}), 400
  #print(int(watts_f_post))
  watts_file(userid_var, deviceid_var, str(watts_f_post))
  return ('', 200)


@app.route('/<path:userid_var>/<path:deviceid_var>/change/')
@jwt_required
def change(userid_var, deviceid_var):
  if not getUserBy( get_jwt_identity(), userid_var ):
    return jsonify({"msg": "not authorized"}), 401
  SetState( userid_var, deviceid_var )
  return ('', 200)

@app.route('/<path:userid_var>/<path:deviceid_var>/state/')
@jwt_required
def def_state(userid_var, deviceid_var):
  if not getUserBy( get_jwt_identity(), userid_var ):
    return jsonify({"msg": "not authorized"}), 401
  return jsonify({"state": GetState( userid_var, deviceid_var )}), 200

if __name__ == '__main__':
  db.create_all()
  app.run( host='0.0.0.0' )

