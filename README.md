# mbash25 - A Mini Bash Shell in C

`mbash25` is a lightweight custom shell written in C, designed to mimic the functionality of standard Unix shells while also supporting several extended features such as redirection, piping, conditional execution, and parallel processing.

---

## 🔧 Features

✅ **Basic Command Execution**  
Execute built-in Unix commands like `ls`, `cat`, `echo`, etc.

✅ **Background Execution**  
Run processes in the background using `&`.  
```bash
sleep 10 &
cat < input.txt  
echo "hello" > file.txt  
echo "again" >> file.txt
cat file.txt | grep hello | wc -l
true && echo "Success"  
false || echo "Fallback"  
ls ; echo "Done"
sleep 5 + ls  
sleep 3 ++ echo "done"
cat file.txt ~ grep error ~ wc -l
ls -l # this lists files


