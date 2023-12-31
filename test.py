import subprocess 
import math
import sys
import time

tol = 0.15

class colors:
    HEADER = '\033[95m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

if len(sys.argv)<2:
    BUILD_TARGET = "main"
else:
    BUILD_TARGET = sys.argv[1]

print(f"{colors.HEADER}{colors.BOLD}{colors.UNDERLINE}Building target {BUILD_TARGET}{colors.ENDC}")
try:
    subprocess.run(["make", BUILD_TARGET], check=True)
except Exception as e:
    print(f"{colors.FAIL}Build Failure.{colors.ENDC}")
    sys.exit(0)
print(f'{colors.OKGREEN}Build complete.{colors.ENDC}')

def run_test(fname, ans):
    print(f"{fname}: ", end="")
    for _ in range(5):
        time.sleep(0.05)
        try:
            out = float(subprocess.run(["./polyvol", "tests/" + fname], capture_output = True, check=True).stdout.decode('utf-8'))
            #print(f"Output: {out} Answer:{ans}") 
            if not abs(out - ans) <= tol * max(out, ans):
                print(f"{colors.FAIL}Failed{colors.ENDC}")
                print(f"{colors.WARNING}Error : {fname}, expected : {ans}, output : {out}{colors.ENDC}")
                return
        except Exception as e:
            print(f"{colors.FAIL}Failed with exception ({type(e)}){colors.ENDC}")
            print(f"{colors.WARNING}Error: {colors.ENDC}{e}")
            return
    print(f"{colors.OKGREEN}Passed{colors.ENDC}")

small_sizes = list(range(1, 11))
big_sizes = [15, 20]
sizes = small_sizes + big_sizes

#Cubes
print(f'{colors.HEADER}{colors.BOLD}{colors.UNDERLINE}Cubes{colors.ENDC}')
for n in sizes:
    ans = 2 ** n
    run_test("cube_" + str(n), ans)
                                                                                                        
print(f'{colors.OKGREEN}Cubes done.{colors.ENDC}')

#Cuboids
print(f'{colors.HEADER}{colors.BOLD}{colors.UNDERLINE}Cuboids{colors.ENDC}')
for n in sizes:
    ans = (2 ** n) * 50
    run_test("cuboid_" + str(n), ans)
                                                                                                        
print(f'{colors.OKGREEN}Cuboids done.{colors.ENDC}')

#Simplices
print(f'{colors.HEADER}{colors.BOLD}{colors.UNDERLINE}Simplices{colors.ENDC}')
for n in sizes:
    ans = 1 / math.factorial(n)
    run_test("simplex_" + str(n), ans) 

print(f'{colors.OKGREEN}Simplices done.{colors.ENDC}')

#Crosses
print(f'{colors.HEADER}{colors.BOLD}{colors.UNDERLINE}Crosses{colors.ENDC}')
for n in small_sizes:
    ans = (2 ** n) / math.factorial(n)
    run_test("cross_" + str(n), ans) 

print(f'{colors.OKGREEN}Crosses done.{colors.ENDC}')
