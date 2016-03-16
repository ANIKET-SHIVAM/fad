import sys, getopt
from os import listdir
from os.path import isfile, join

def main(argv):	
	PATH_BLACKHOLE="/home/aniket/fad/blackhole"
	try:
		PATH_BLACKHOLE_SEEDS="/home/aniket/fad/blackhole/seeds"
		CLIENTS = 2
		opts, args = getopt.getopt(argv,"c:i:")
		for opt, arg in opts:
			if opt == '-c':
				CLIENTS = int(arg)
			if opt == "-i":
				PATH_BLACKHOLE_SEEDS = arg
	except getopt.GetoptError:
		PATH_BLACKHOLE_SEEDS="/home/aniket/fad/blackhole/seeds"
		CLIENTS = 2

	seed_files = [f for f in listdir(PATH_BLACKHOLE_SEEDS) if isfile(join(PATH_BLACKHOLE_SEEDS, f))]
	list_infiles=[]	
	list_inputs=[]
	
	for i in range(len(seed_files)):      
		list_infiles.append(open(PATH_BLACKHOLE_SEEDS+"/"+seed_files[i],'r'))
	outfile = open(PATH_BLACKHOLE+"/offspring.c",'w')

	for i in range(len(list_infiles)):
		list_inputs.append(list_infiles[i].readlines())

	#fetch lines for preprocessing 
	def preprocessings(input):
		for line in input:
			if line.startswith("#define"):
				outfile.write(line)

	#fetch global variables
	def globalvars(input):
		for line in input:
			if line.startswith("int main"):
				return
			if not line.startswith('#') and not line.startswith('//') and not line.startswith('\n'):
				outfile.write(line)

	#put the kernels inside functions
	def kernels(input):
		flag = False
		for line in input:
			if flag:
				outfile.write(line)
			if not flag and line.startswith("int main"):
				flag = True

	"""MAIN OUTFILE DEFINATION"""

	outfile.write("#include <stdio.h>" + '\n')
	outfile.write('\n')
	for i in range(len(list_inputs)):
		preprocessings(list_inputs[i])
	outfile.write('\n')
	for i in range(len(list_inputs)):
		preprocessings(list_inputs[i])
		globalvars(list_inputs[i])
	outfile.write('\n')
	for i in range(len(list_inputs)):
		preprocessings(list_inputs[i])
		outfile.write("int func"+str(i)+"() {"  + '\n')
		kernels(list_inputs[i])
		outfile.write('\n')
	outfile.write("int main() {"  + '\n')
	for i in range(len(list_inputs)):
		preprocessings(list_inputs[i])
		outfile.write("printf(\"RESULT_FUNC_"+str(i)+"%d\\n\",func"+str(i)+"());" + '\n')
	outfile.write("return 0;" + '\n')
	outfile.write("}" + '\n')

if __name__ == "__main__":
   main(sys.argv[1:])
