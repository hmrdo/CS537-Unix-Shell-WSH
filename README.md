# Unix Shell

## 🔍 Problem Statement  
In this project, I implemented a Unix shell called **`wsh`**, capable of running both **interactive and batch modes**. The shell supports:  
- **Built-in commands**: `exit`, `cd`, `local`, `export`, and `vars`.  
- **Process execution**: Using `fork()` and `execv()` to run external programs.  
- **Input/output redirection**: Handling `<` and `>` for file-based input and output.  
- **Environment and shell variables**: Supporting local and exported variables.  
- **Script execution**: Running scripts with `#!./wsh`.  

The project required careful **command parsing, process management, and memory safety checks**, ensuring that `wsh` behaves similarly to a standard Unix shell.  

---  

## 🎯 What I Learned  
This assignment provided valuable experience in **systems programming and shell design**. Key takeaways include:  
✅ **Process creation and execution** – Using `fork()` and `execv()` to manage child processes.  
✅ **File redirection** – Implementing `<` and `>` to manipulate stdin and stdout.  
✅ **Shell variable handling** – Managing local and environment variables correctly.  
✅ **Script execution** – Understanding how the shell interprets and runs script files.  
✅ **Memory safety and debugging** – Avoiding memory leaks and ensuring robust error handling.  

---  

## 🏆 Results  
I successfully implemented all required features of `wsh`, passing **all test cases**, and received **100% on the assignment**. This project strengthened my understanding of **process control, shell internals, and Unix system programming**.  

---  

## 📟 How to Run
To run this shell, simply clone this repository to anywhere on your computer and run ./wsh. (You need to be in a unix environment or running bash, zsh, or something similar).

Once you've ran the shell, then you can do whatever there is to do on your previous shell!
```bash
[hdoll@royal-22] (33)$ ./wsh
wsh> echo hello world
hello world
wsh>
```
