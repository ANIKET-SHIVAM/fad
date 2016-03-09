
infile1 = open("/home/aniket/fad/blackhole/test0.c",'r')
infile2 = open("/home/aniket/fad/blackhole/test1.c",'r')
outfile = open("/home/aniket/fad/blackhole/offspring.c",'w')

input1 = infile1.readlines()
input2 = infile2.readlines()

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
preprocessings(input1)
preprocessings(input2)
outfile.write('\n')
globalvars(input1)
globalvars(input2)
outfile.write('\n')
outfile.write("int func1() {"  + '\n')
kernels(input1)
outfile.write('\n')
outfile.write("int func2() {"  + '\n')
kernels(input2)
outfile.write('\n')
outfile.write("int main() {"  + '\n')
outfile.write("int result1 = func1();" + '\n')
outfile.write("int result2 = func2();" + '\n')
outfile.write("return 0;" + '\n')
outfile.write("}" + '\n')
