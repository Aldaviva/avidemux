# check get_folder_content function
# return the list of files with extention ext
# full path is returned !
# i.e. in the example below you will get a list
# /work/samples/avi/foo.avi
# /work/samples/avi/bar.avi
# ...
#
import os
print("environ ")
val=os.environ("foo")
print("val:"+str(val))
print("Done")
