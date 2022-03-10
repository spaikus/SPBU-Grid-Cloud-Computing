import os
from distutils.log import debug
from flask import Flask, request, render_template
from flask_restx import Api, Resource
import hash

hash_funcs = {
    'sha256': hash.sha256,
    'streebog256': hash.streebog256
}
hash_funcs_list = list(hash_funcs.keys())


app = Flask(__name__)

@app.route('/', methods=['GET', 'POST'])
def page():
    input_str = ''
    hash_func = hash_funcs_list[0]

    if request.method == "POST":
        input_str = request.form['input_str']
        hash_func = request.form['hash_func']
    
    hash = hash_funcs[hash_func](input_str)
    return render_template('page.html', input_str=input_str, hash=hash, hash_func=hash_func, hash_funcs=hash_funcs_list)


api = Api(app, doc='/api/', version='1.0', title='HashGen', description='256-bit hash generator supporting sha-2/streebog hash functions')
apins = api.namespace('hash', description='Hash generator')

@apins.route('/')
@apins.doc(params={'hash_func':'Hash function', 'str':'Input string to hash'})
class HashGen(Resource):
    @apins.doc(responses={200:'Success', 404:'Invalid hash function'})
    def get(self):
        """Generate hash for input string using selected hash function"""
        try:
            hash_func = hash_funcs[request.args['hash_func']]
        except:
            return 'Invalid hash function', 404

        input_str = ''
        if 'str' in request.args:
            input_str = request.args['str']
        return hash_func(input_str)

@apins.route('/funcs/')
class ListHashes(Resource):
    def get(self):
        '''List all available hash functions'''
        return list(hash_funcs.keys())

if __name__ == '__main__':
    app.run()