scite-ru-src
============

给scite-ru添加新增设置行间距功能。



##关键代码
在`/src/scite/src/SciTEProps.cxx`中添加如下内容：

```c
int iAscent = 0;
int iDescent = 0;
int lineSpacing = props.GetInt("line.spacing", 2);
if(lineSpacing < 0){
    lineSpacing = 0;
}
if(lineSpacing > 64){
    lineSpacing = 64;
}
if (lineSpacing % 2) {
    iAscent++;
    lineSpacing--;
}
iAscent += lineSpacing / 2;
iDescent += lineSpacing / 2;
wEditor.Call(SCI_SETEXTRAASCENT, iAscent);
wEditor.Call(SCI_SETEXTRADESCENT, iDescent);
```

