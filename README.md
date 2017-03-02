# enhanced-linux-shell
Enhanced version of previous linux shell repository. 

Project Description: 
This program creates a shell environment that is an extension of the shell environment created in Project 3. The shell can take in 1024 characters, and can hold 512 arguments. The user can also type in more than one command (if there were to use a pipeline). If this occurs, the command array can hold 512 different commands. 

There is also a path variable that can hold 512 different paths. The program works by taking in the user input and parsing it, separating it into individual commands. The program handles three internal commands (cd, path, and quit) and executes these commands in the parent process. If a command other than any of these is typed (AND THERE IS ONLY ONE COMMAND TYPED), the command will be executed in a child process if it is found. The program "finds" these commands by searching through each directory in the path variable. 

However, if more than one command is typed, the parent shell program branches off into a "pseudo-parent" that will create all of the pipes and that will branch off all of the children. The input and output of the commands are changed if necessary.

Also, this shell allows for input and output redirection if "<" or ">" is typed. 

NOTE: When using pipelines, there is a slight delay after typing the input before the output appears. This is because the parent process for pipelining waits for each child (command that is in the pipeline) to either execute or exit. If it exits, it will generate a signal that will be caught by the parent. The slight time delay gives the parent process enough time to catch and handle the signal before forking again and making more children. 
