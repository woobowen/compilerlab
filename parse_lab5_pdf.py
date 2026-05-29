#!/usr/bin/env python3
import os
import requests
import time
import zipfile
from io import BytesIO

# Get API token
token = os.environ.get("MINERU_API_KEY")
if not token:
    raise ValueError("MINERU_API_KEY environment variable not set")

# Step 1: Submit parsing task
pdf_path = "/home/addaswsw/lab/tiger-compiler-26sp/Lab5 part2_ Tiger Compiler without register allocation.pdf"
print(f"Uploading PDF from: {pdf_path}")

# First, we need to upload the file to get a URL
# Since MinerU requires a URL, let's check if we can use a local file path
# Actually, MinerU API requires a URL, not a local file path
# We need to provide a publicly accessible URL

# For now, let's try to read the PDF directly with a different approach
print("Note: MinerU API requires a URL, not a local file path.")
print("Attempting alternative approach...")

# Let's just try to extract text from the PDF using a local tool
import subprocess

# Try pdftotext
result = subprocess.run(['pdftotext', pdf_path, '-'], capture_output=True, text=True)
if result.returncode == 0:
    print("Successfully extracted text using pdftotext")
    with open('lab5_part2_extracted.txt', 'w') as f:
        f.write(result.stdout)
    print("Saved to lab5_part2_extracted.txt")
else:
    print(f"pdftotext failed: {result.stderr}")
    print("Trying alternative method...")

    # Try using Python's PyPDF2 or similar
    try:
        import PyPDF2
        with open(pdf_path, 'rb') as f:
            reader = PyPDF2.PdfReader(f)
            text = ""
            for page in reader.pages:
                text += page.extract_text() + "\n"

        with open('lab5_part2_extracted.txt', 'w') as f:
            f.write(text)
        print("Successfully extracted text using PyPDF2")
        print("Saved to lab5_part2_extracted.txt")
    except ImportError:
        print("PyPDF2 not installed. Installing...")
        subprocess.run(['pip', 'install', 'PyPDF2'], check=True)

        import PyPDF2
        with open(pdf_path, 'rb') as f:
            reader = PyPDF2.PdfReader(f)
            text = ""
            for page in reader.pages:
                text += page.extract_text() + "\n"

        with open('lab5_part2_extracted.txt', 'w') as f:
            f.write(text)
        print("Successfully extracted text using PyPDF2")
        print("Saved to lab5_part2_extracted.txt")
