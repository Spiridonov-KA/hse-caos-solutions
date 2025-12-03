import random
from tqdm import tqdm

UINT32_MAX = (1 << 32) - 1

def compute(a, n, m):
    if m == 1:
        return 0
    res = a % m
    for _ in range(n):
        res = (res * res) % m
    return res

def generate_test(test_num, t, queries, time_limit=1500):
    padded = f"{test_num:03d}"
    with open(f"tests/{padded}.dat", "w") as f:
        # f.write(f"{t}\n")
        for a, n_val, m in queries:
            f.write(f"{a} {n_val} {m}\n")
    answers = []
    for i, (a, n_val, m) in tqdm(enumerate(queries, 1)):
        ans = compute(a, n_val, m)
        answers.append(f"<{i}> {ans}")
    with open(f"tests/{padded}.ans", "w") as f:
        f.write("\n".join(answers) + "\n")
    with open(f"tests/{padded}.inf", "w") as f:
        f.write(f"real_time_limit_ms = {time_limit}\n")

random.seed(42)

# Test 001: small, t=4, few small tasks
queries_001 = [
    (2, 1, 5),
    (3, 2, 7),
    (4, 3, 13)
]
generate_test(1, 2, queries_001, time_limit=10)

# Test 002: t=4, many small tasks (n=1000, 4000 tasks)
queries_002 = []
for _ in range(2000):
    a = random.randint(0, UINT32_MAX)
    n = 10000
    m = random.randint(2, UINT32_MAX)
    queries_002.append((a, n, m))
generate_test(2, 4, queries_002, time_limit=100)

# Test 003: t=4, many big tasks (8 tasks, n=10000000)
queries_003 = []
for _ in range(8):
    a = random.randint(0, UINT32_MAX)
    n = 10000000
    m = random.randint(2, UINT32_MAX)
    queries_003.append((a, n, m))
generate_test(3, 4, queries_003, time_limit=200)

# Test 005: t=4, different tasks (10 tasks, varying n)
queries_004 = []
n_values = [10, 10, 10, 100, 100, 1000, 1000, 10000, 10000, 100000, 1000000, 10000000, 100000000, 100000000]
for nv in n_values:
    a = random.randint(0, UINT32_MAX)
    m = random.randint(2, UINT32_MAX)
    queries_004.append((a, nv, m))
generate_test(4, 4, queries_004, time_limit=2000)