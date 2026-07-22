#!/usr/bin/env python3
import sys
import os

# 1. Strict HTTP formatting: The double \r\n separates headers from the body.
# 1. 严格的 HTTP 格式：双 \r\n 用于分隔头部和主体。
sys.stdout.write("Content-Type: text/html\r\n\r\n")

# 2. Start HTML body
# 2. 开始 HTML 主体
sys.stdout.write("<html><body>\n")
sys.stdout.write("<h1>CGI Debug Script Successful!</h1>\n")
sys.stdout.write("<hr>\n")

# 3. Handle GET requests (QUERY_STRING)
# 3. 处理 GET 请求 (QUERY_STRING)
query_string = os.environ.get('QUERY_STRING', '')
if query_string:
    sys.stdout.write(f"<p><strong>GET Query String Detected:</strong> {query_string}</p>\n")

# 4. Handle POST requests (Read from stdin based on CONTENT_LENGTH)
# 4. 处理 POST 请求 (基于 CONTENT_LENGTH 从 stdin 读取)
content_length = os.environ.get('CONTENT_LENGTH')
if content_length and content_length.isdigit() and int(content_length) > 0:
    # Read exactly the number of bytes specified / 准确读取指定的字节数
    body = sys.stdin.read(int(content_length))
    sys.stdout.write(f"<p><strong>POST Received Payload:</strong> {body}</p>\n")
elif os.environ.get('REQUEST_METHOD') == 'POST':
    sys.stdout.write("<p><strong>POST Request received but no Content-Length found.</strong></p>\n")

# 5. Close HTML body
# 5. 关闭 HTML 主体
sys.stdout.write("</body></html>\n")
