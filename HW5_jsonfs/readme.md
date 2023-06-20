# Operating Systems Homework5


* 21800637 Jooyoung Jang




---

# Homework 5. JsonFS using FUSE
 

JsonFS is a program to implement file system at userlevel based on a strucuture that were provided with json format.

# How to use this Program

### Preprocess

1. clone this repository using:

```
$ git clone https://github.com/0neand0nly/OS_23_1.git
then cd to HW5_jsonfs
```

2. Compile and run
   This repository provides example test programs. To run them, use ``./build.sh`` to compile. This will produce jsonfs exectuable file.

   use ``./run.sh``




# Program Overview


  
INPUT FILE



```
[
	{
		"inode": 0,
		"type": "dir",
		"entries":
		[
			{"name": "hello", "inode": 1},
			{"name": "d1", "inode": 2}
		]
	},
	{
		"inode": 1,
		"type": "reg",
		"data": "Hello World!"
	},
	{
		"inode": 2,
		"type": "dir",
		"entries":
		[
			{"name": "d2", "inode": 3}
		]
	},
	{
		"inode": 3,
		"type": "dir",
		"entries":
		[
			{"name": "bye", "inode": 4},
			{"name": "hello", "inode": 1}
		]
	},
	{
		"inode": 4,
		"type": "reg",
		"data": "come!"
	},
	{
		"inode": 5,
		"type": "reg",
		"data": "Goodbye!"
	}
]

...
```

# Expected Output
```
++ mkdir test
++ ./jsonfs fs.json ./test
Created file object with inode 0, type dir
Entries:
inode: 1, name: hello
inode: 2, name: d1
Created file object with inode 1, type reg
Created file object with inode 2, type dir
Entries:
inode: 3, name: d2
Created file object with inode 3, type dir
Entries:
inode: 4, name: bye
inode: 1, name: hello
Created file object with inode 4, type reg
Created file object with inode 5, type reg
```


