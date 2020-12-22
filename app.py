from flask import Flask, jsonify, request
import os
from flask_jwt_extended import JWTManager, jwt_required, create_access_token, get_jwt_identity
from werkzeug.security import generate_password_hash, check_password_hash
from flask_sqlalchemy import SQLAlchemy
import uuid
import json

basedir = os.path.abspath(os.path.dirname(__file__))

def uuid_gen():
  return str(uuid.uuid4().node)

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///' + os.path.join(basedir, 'database.sqlite')
app.config['SECRET_KEY'] = "you-will-never-guess"

db = SQLAlchemy(app)

app.config['JWT_SECRET_KEY'] = 'super_super-secret'
jwt = JWTManager(app)

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

file='file.txt'

def SetState():
  actual =  GetState()
  f_write = open(file, "w")
  if actual == 'False':
    f_write.write("True")
  else:
    f_write.write("False")
  f_write.close()

def GetState():
  if not os.path.exists(file):
    f_init = open(file, "w")
    f_init.write("False")
    f_init.close()
  f_open = open(file, "r")
  return f_open.read()

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

@app.route('/protected')
@jwt_required
def protected():
    return '%s' % get_jwt_identity()

@app.route('/change')
def change():
  SetState()
  return ('', 200)

@app.route('/state')
def def_state():
  return jsonify({"state": GetState()}), 200

if __name__ == '__main__':
  db.create_all()
  app.run( host='0.0.0.0' )

