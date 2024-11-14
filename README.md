# vip
Small, fast and featureful vim-insipired text editor with strictly no dependency.

It has colorful syntax highlighting and integrate with [ccc](https://github.com/night0721/ccc) for picking files

# Usage
```sh
vip -c file # Cat mode
vip file
```

```
LEFT/h: left
DOWN/j: down
UP/k: up
RIGHT/l: right
HOME/0: start of line
END/$: end of line
PAGEUP: jump up a page
PAGEDOWN: jump down a page
Backspace/ctrl+h/del: delete char
:w: save file
:q(!): quit
i: insert mode
v: visual mode
ESC: normal mode
gg: start of file
G: end of file
dd: delete line
/: search
O: create new line above cursor
o: create new line below cursor
 pv: open ccc

```
# Dependencies
None

# Building
You will need to run these with elevated privilages.
```
$ make 
# make install
```

# Contributions
Contributions are welcomed, feel free to open a pull request.

# License
This project is licensed under the GNU Public License v3.0. See [LICENSE](https://github.com/night0721/vip/blob/master/LICENSE) for more information.
