from pathlib import Path
import re,json
# A KeyValues reader for source extraction only; preserves all ordered duplicate pairs.
def parse(text):
 tokens=re.finditer(r'//[^\n]*|"((?:\\.|[^"\\])*)"|([{}])|([^\s{}"]+)',text); it=iter(tokens)
 def token():
  for m in it:
   if m.group(0).startswith('//'):continue
   if m.group(1) is not None:return m.group(1).replace(r'\"','"').replace(r'\\','\\')
   return m.group(2) or m.group(3)
  return None
 def block(n=0):
  if n>96:raise ValueError('depth')
  out={}
  while True:
   k=token()
   if k in (None,'}'):return out
   v=token()
   if v=='{':v=block(n+1)
   if k in out and isinstance(out[k],dict) and isinstance(v,dict):out[k].update(v)
   else:out[k]=v
 return block()
