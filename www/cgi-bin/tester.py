#!/usr/bin/env python3
sys = __import__('sys')
os = __import__('os')

print("Content-Type: text/html\r\n")
print()

print("<html><body>")
print("<h1>POST Request Received!</h1>")

content_length = os.environ.get('CONTENT_LENGTH')
if content_length:
    body = sys.stdin.read(int(content_length))
    print(f"<p>Received Payload: {body}</p>")
else:
    print("<p>No Content-Length found.</p>")

print("</body></html>")