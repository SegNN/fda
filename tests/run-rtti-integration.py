from pathlib import Path
import tempfile,shutil,subprocess
r=Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory(prefix='fda-rtti-') as name:
 p=Path(name)
 for f in ('rtti.cpp','rtti_client.inc','runtime_rtti.h'):shutil.copy(r/'src'/f,p/f)
 for f in ('mem.h','test.cpp'):shutil.copy(r/'tests/rtti-integration-fixture'/f,p/f)
 subprocess.run(['g++','-std=c++17','-O1','-pthread',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
