import os

print("Content-Type: text/plain\r\n")
for k, v in sorted(os.environ.items()):
    print(f"{k}={v}")
