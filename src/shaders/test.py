def parse_gl_file(filename):
    sf = open(filename, "r")    

    line = sf.readline()

    while line:
        if line.find("#ifdef") != -1:
            print("hello")
        
        line = sf.readline()
    sf.close()

def print_hello():
    print("hello")


parse_gl_file("dof.comp")
