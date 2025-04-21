import os
import sys
import pathlib
import subprocess

def main():
  print(sys.argv)
  default_depot_tools = str(pathlib.Path.home() / 'depot_tools')
  depot_tools = os.environ.get('DEPOT_TOOLS', default_depot_tools)

  new_env = dict(os.environ)
  new_env['PATH'] = depot_tools + os.pathsep + new_env['PATH']

  subprocess.run(sys.argv[1:], env=new_env)

if __name__ == '__main__':
  main()
