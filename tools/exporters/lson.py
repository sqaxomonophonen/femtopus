# lua simple object notation (or whatever)

def dumps(obj, comment = None):
	def e0(v):
		if v is None:
			return "nil"
		elif isinstance(v, bool):
			if v:
				return "true"
			else:
				return "false"
		elif isinstance(v, (int, float)):
			return "%s" % v
		elif isinstance(v, str):
			return repr(v)
		else:
			raise ValueError(v)

	def e1(v):
		if isinstance(v, (list, set, tuple)):
			return "{%s}" % ",".join(map(e1, v))
		elif isinstance(v, dict):
			return "{%s}" % ",".join(map(lambda item: "[%s]=%s" % (e0(item[0]), e1(item[1])), v.items()))
		else:
			return e0(v)

	out = ""
	if comment:
		out += "-- %s\n" % comment
	out += "return %s\n" % e1(obj)
	return out
