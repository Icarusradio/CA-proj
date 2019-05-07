from pathlib import Path

if __name__ == '__main__':
	p = Path('./dress-jpeg')
	maxSize = 1048576
	for imageFile in p.glob('*.jpg'):
		fileName = str(imageFile)
		stat = imageFile.stat()
		if stat.st_size > maxSize:
			print("Remove", fileName[12:])
			imageFile.unlink()
		if stat.st_size % 8192 == 0:
			print("Remove", fileName[12:])
			imageFile.unlink()
