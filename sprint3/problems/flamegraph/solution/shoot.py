import argparse
import subprocess
import time
import random
import shlex
import os

RANDOM_LIMIT = 1000
SEED = 123456789
FLAMEGRAPH_PATH = './FlameGraph'
random.seed(SEED)

AMMUNITION = [
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1

def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server

def run(command, output=None):
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.DEVNULL)
    return process

def stop(process, wait=False):
    if process.poll() is None and wait:
        process.wait()
    process.terminate()

def shoot(ammo):
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)

def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')

def record_perf(pid):
    print(f"Starting perf record for PID {pid}")
    return run(f'perf record -p {pid} -g -o perf.data')

def generate_flamegraph():
    if os.path.exists("perf.data") and os.path.getsize("perf.data") > 0:
        with open("perf.data", "rb") as perf_data:
            p1 = subprocess.Popen(['perf', 'script'], stdin=perf_data, stdout=subprocess.PIPE)
            with open("graph.svg", "wb") as svg_file:
                p2 = subprocess.Popen(
                    ['perl', os.path.join(FLAMEGRAPH_PATH, 'stackcollapse-perf.pl')],
                    stdin=p1.stdout,
                    stdout=subprocess.PIPE
                )
                p3 = subprocess.Popen(
                    ['perl', os.path.join(FLAMEGRAPH_PATH, 'flamegraph.pl')],
                    stdin=p2.stdout,
                    stdout=svg_file
                )
                p1.stdout.close()
                p2.stdout.close()
                p3.communicate()
        print("Flamegraph generated: graph.svg")
    else:
        print("Error: perf.data is empty. No data to generate flamegraph.")

server = run(start_server())
perf_process = record_perf(server.pid)
make_shots()
stop(server)
time.sleep(1)
generate_flamegraph()
print('Job done')
