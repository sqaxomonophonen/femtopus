#!/usr/bin/env python3
import http.server
import os
import json

os.chdir(os.path.dirname(__file__))

class State:
	def get_selection(self):
		return {
			"path": "//../materials/m0.blend", # XXX should be absolute
			"name": "extmat1"
		}

state = State()

class Handler(http.server.SimpleHTTPRequestHandler):
	def rq(self, method):
		if self.path.startswith("/static/"):
			getattr(super(), "do_%s" % method)()
		elif self.path.startswith("/api/get_selection"):
			state_json = json.dumps(state.get_selection()).encode('utf-8')
			self.send_response(200)
			self.send_header('Content-Type', 'application/json')
			self.end_headers()
			self.wfile.write(state_json)
		else:
			self.send_response(404)
			self.end_headers()
			self.wfile.write(b"404 Not Found")

	def do_GET(self):
		self.rq("GET")

	def do_HEAD(self):
		self.rq("HEAD")

http.server.HTTPServer(('', 6510), Handler).serve_forever()
