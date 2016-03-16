import sys, getopt
def main(argv):	
	try:
		PATH_BLACKHOLE="/home/aniket/fad/blackhole"
		CLIENTS = 2
		opts, args = getopt.getopt(argv,"c:i:")
		for opt, arg in opts:
			if opt == '-c':
				CLIENTS = int(arg)
			if opt == "-i":
				PATH_BLACKHOLE = arg
	except getopt.GetoptError:
		PATH_BLACKHOLE="/home/aniket/fad/blackhole"
		CLIENTS = 2
	      
	infile1 = open(PATH_BLACKHOLE+"/aftermath",'r')

	input1 = infile1.readlines()
	
	client_results = []
	for line in input1:
		if line.startswith("# RESULT_FUNC_"):
			client_results.append(line[len("# RESULT_FUNC_")+1:-1])
	for i in range(1,CLIENTS+1):
		print("Client "+str(i)+":"+client_results[i-1])
if __name__ == "__main__":
   main(sys.argv[1:])
