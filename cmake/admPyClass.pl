#
# Generate glue to hook C functions to tinyPy static classes
# returnvalue FUNC param
#       param [int, float, void, str, couples]
#                        str = const char *
#                        couples =  confCouple *c
#
# If we deal with instance, the method must be set through set/get as metadata
#


use strict;
my $input;
my $output;
my $headerFile;
my $line;
my $glueprefix="zzpy_";
my $functionPrefix="";
my $className;
my $cookieName;
my $cookieId;
my $staticClass=0;
#
my %cFuncs;
my %rType;
my %funcParams;
my @ctorParams;
#
# Variables
my %getVar;
my %setVar;
my %typeVars;

sub debug
{
        my $str=shift;
        chomp($str);
#        print OUTPUT "jsLog(\"$str\\n\");\n";

}
#
# processClass
#
sub processClass
{
        my $proto=shift;

        $proto=~s%.*\*/%%g;
        $proto=~s/ //g;
        #print "**$proto**\n";
        ($className,$cookieName,$cookieId)=split ":",$proto;
        print "Processing class $className (with cookie=$cookieName,id=$cookieId)\n";
        if($cookieName=~m/void/)
        {
                $staticClass=1;
        }
        else
        {
                $staticClass=0;
        }

}
#
# Process the constructor list
#
sub processCTOR
{
        my $proto=shift;
        my $args=$proto;
        my @params;
        my $retType=$proto;;
        my $pyfunc=$proto;;
        my $cfunc;
        $args=~s/^.*\(//g;
        $args=~s/\).*$//g;
        #print "args => $args\n";
        @ctorParams=split ",",$args;
        #print " $pyfunc -> @params \n";
}
#
# Process a function declaration and write the glue code to do tinypy<->function call
#
sub processMETHOD
{
        my $proto=shift;
        my $args=$proto;
        my @params;
        my $retType=$proto;;
        my $pyfunc=$proto;;
        my $cfunc;
        $args=~s/^.*\(//g;
        $args=~s/\).*$//g;
        #print "args => $args\n";
        @params=split ",",$args;
        #print "params -> @params\n";
        # Get return type...
        $retType=~s/^ *//g;
        $retType=~s/ .*$//g;
        # get functioName
        $pyfunc=~s/ *\(.*$//g;
        $pyfunc=~s/^.* //g;
        ($cfunc,$pyfunc)=split ":",$pyfunc;
        $cFuncs{$pyfunc}=$cfunc;
        $rType{$pyfunc}=$retType;

        if ($staticClass == 1 && $cfunc !~ /editor->/)
        {
            if (@params[0] =~ m/^void$/)
            {
                @params = ();
            }

            unshift @params, "IEditor";
        }

        push @{$funcParams{$pyfunc}}, @params;
        #print " $pyfunc -> @params \n";
}

#
# Process a  STATICVAR :/* STATICVAR */ int    audioResample:scriptSetResample,scriptGetResample
#
sub processStaticVar
{
        my $proto=shift;
        my $args=$proto;
        my $type;
        my $varname;
        my @left;
        my @funcs;


        $args=~s/^.*\*\///g; # Remove comment

        @left=split(":",$args);
        @funcs=split(",",$left[1]);
        #print " >> ".$left[0]."<<\n";
        ($type,$varname)=split(" ",$left[0]);

        $type=~s/ //g;
        $varname=~s/ //g;

        #print "<$type> * <$varname> * <@funcs> *\n";
        $typeVars{$varname}=$type;
        $setVar{$varname}=$funcs[0];
        $getVar{$varname}=$funcs[1];
        print "Processing variable $varname of type $type\n";
}


#
# parsefile
#
sub parseFile
{
my $file=shift;
open(INPUT,$file) or die("Cannot open $file");
while($line=<INPUT>)
{
        chomp($line);
        if($line=~m/^#/)
        {
        }
        else
        {
            if($line=~m/\* METHOD \*/)
            {
                my $proto=$line;
                # Remove header...
                # Remove tail
                $line=~s/^.*\*\///g;
                $line=~s/\/\/.*$//g;
                processMETHOD($line);

            } elsif($line =~m/\/* CLASS \*/)
                {
                        processClass($line);
                }
                elsif($line =~m/\/* ATTRIBUTES \*/)
                {
                        processStaticVar($line);
                }
                elsif($line=~m/\/* CTOR \*/)
                {
                        processCTOR($line);
                }
}
}
close(INPUT);
}
#
#
#
sub genReturn
{
        my $retType=shift;
        if($retType=~m/int/)
        {
                return "  return tp_number(r);";
        }
        if($retType=~m/double/)
        {
                return "  return tp_number(r);";
        }
        if($retType=~m/float/)
        {
                return "  return tp_number(r);";
        }
        if($retType=~m/void/)
        {
                return " return tp_None;";
        }
         if($retType=~m/str/)
        {
                return "  if(!r) pm.raise(\"$className : null pointer\");\n
  tp_obj o = tp_string_copy(tp, r, strlen(r));
  ADM_dealloc(r);
  return o;";
        }
return "???? $retType";
}
#
#
#
sub castFrom
{
        my $type=shift;
        if($type=~m/^str/)
        {
                return "pm.asString()";
        }
        if($type=~m/^int/)
        {
                return "pm.asInt()";
        }
        if($type=~m/^float/)
        {
                return "pm.asFloat()";
        }
        if($type=~m/^ptr/)
        {
                my $ptr;
                my $obj;
                ($ptr,$obj)=split "@",$type;
                return "( $obj *)pm.asObjectPointer()";
        }
        if($type=~m/^double/)
        {
                return "pm.asDouble()";
        }
        if($type=~m/^IEditor/)
        {
            return "editor";
        }
        print ">>>>>>>>>> <$type> unknown\n";
        return "????";

}
#
# Generate the locals / wrapping for the parameters
#
sub genParam
{
        my $num=shift;
        my $type=shift;
        my $r=1;
        $type=~s/ *$//g;
        $type=~s/^ *//g;
        if($type=~m/couples/)
        {
                print OUTPUT "  CONFcouple *p".$num." = NULL;\n";
                print OUTPUT "  pm.makeCouples(&p".$num.");\n";
                $r=0;
        }else
        {
                my $regular=1;
                if($type=~m/^str/)
                {
                        print OUTPUT "  const char *p".$num." = ";
                        $regular=0;
                }
                if($type=~m/^ptr/)
                {
                        $regular=0;
                        my $ptr;
                        my $obj;
                        ($ptr,$obj)=split "@",$type;

                        print OUTPUT "  $obj *p".$num." = ";

                }
                if($type =~ m/^IEditor/)
                {
                        print OUTPUT "  $type *p".$num." = ";
                        $regular=0;
                }
                if($regular==1)
                {
                        print OUTPUT "  $type p".$num." = ";
                }
                print OUTPUT castFrom($type);
                print OUTPUT ";\n";
        }
        return $r;
}
#
# Generate getter and setter glue
#
sub genGetSet
{
#
# Generate get/set
#
        my $i;
        my $f;
        my $setName=$glueprefix."_".$className."_set";
        my $getName=$glueprefix."_".$className."_get";
# Get
        print OUTPUT "tp_obj ".$getName."(tp_vm *vm)\n";
        print OUTPUT "{\n";
        debug("$getName invoked\n");
        print OUTPUT "  tp_obj self = tp_getraw(vm);\n";
        print OUTPUT "  IScriptEngine *engine = (IScriptEngine*)tp_get(vm, vm->builtins, tp_string(\"userdata\")).data.val;\n";
        print OUTPUT "  IEditor *editor = engine->editor();\n";
        print OUTPUT "  TinyParams pm(vm);\n";
        print OUTPUT "  $cookieName *me=($cookieName *)pm.asThis(&self, $cookieId);\n";
        print OUTPUT "  char const *key = pm.asString();\n";
        my @k=keys %typeVars;
        my $v;
        foreach $v (@k)
        {
			print OUTPUT "  if (!strcmp(key, \"$v\"))\n";
			print OUTPUT "  {\n";

			if($staticClass==1)
			{
                print OUTPUT "     return tp_number(".$getVar{$v}."(";

                if ($getVar{$v} !~ m/^editor/)
				{
					print OUTPUT "editor";
				}

                print OUTPUT "));\n";
			}
			else
			{
                print OUTPUT "     if(!me) pm.raise(\"$className:No this!\");\n";
                print OUTPUT "     return tp_number(me->".$getVar{$v}."());\n";
			}

			print OUTPUT "  }\n";
        }
        #
        # Functions are part of it...
        #
        my $cfunk;
        foreach $f(  keys %cFuncs)
        {
                my $pyFunc=$f;
                $cfunk=$glueprefix.$f;
                print OUTPUT "  if (!strcmp(key, \"$pyFunc\"))\n";
                print OUTPUT "  {\n";
                print OUTPUT "     return tp_method(vm, self, ".$cfunk.");\n";
                print OUTPUT "  }\n";

        }
#print OUTPUT "  pm.raise(\"No such attribute %s\",key);\n";
        print OUTPUT "  return tp_get(vm, self, tp_string(key));\n";
        print OUTPUT "}\n";

# Set
        print OUTPUT "tp_obj ".$setName."(tp_vm *vm)\n";
        print OUTPUT "{\n";
        debug("$setName invoked\n");
        print OUTPUT "  tp_obj self = tp_getraw(vm);\n";
        print OUTPUT "  IScriptEngine *engine = (IScriptEngine*)tp_get(vm, vm->builtins, tp_string(\"userdata\")).data.val;\n";
        print OUTPUT "  IEditor *editor = engine->editor();\n";
        print OUTPUT "  TinyParams pm(vm);\n";
        print OUTPUT "  $cookieName *me = ($cookieName *)pm.asThis(&self, $cookieId);\n";
        print OUTPUT "  char const *key = pm.asString();\n";
        my @k=keys %typeVars;
        my $v;
        foreach $v (@k)
        {
          print OUTPUT "  if (!strcmp(key, \"$v\"))\n";
          print OUTPUT "  {\n";
        if($staticClass==1)
          {
                print OUTPUT "     ".$typeVars{$v}." val = ".castFrom($typeVars{$v}).";\n";
                print OUTPUT "     ".$setVar{$v}."(";

				if ($setVar{$v} !~ m/^editor/)
				{
					print OUTPUT "editor, ";
				}

                print OUTPUT "val);\n";
                print OUTPUT "     return tp_None;\n";
           }else
          {
                print OUTPUT "     if(!me) pm.raise(\"$className:No this!\");\n";
                print OUTPUT "     ".$typeVars{$v}." val = ".castFrom($typeVars{$v}).";\n";
                print OUTPUT "     me->".$setVar{$v}."(val);\n";
                print OUTPUT "     return tp_None;\n";

         }
          print OUTPUT "  }\n";
        }
    #if (!strcmp(key, "prize")) return tp_number(fruit->prize);
        #print OUTPUT "  pm.raise(\"No such attribute %s\",key);\n";
        print OUTPUT "  return tp_None;\n";
        print OUTPUT "}\n";


}
#
# Gen glue
#
sub genGlue
{
#
#  Generate function call glue
#
#
#
#
        my $i;
        my $f;
        foreach $f(  keys %cFuncs)
        {
                my @params=@{$funcParams{$f}};
                my $pyFunc=$f;
                my $cfunc=$cFuncs{$f};
                my $ret=$rType{$f};
                my $nb=scalar(@params);

                print "Generating $pyFunc -> $ret $cfunc (@params ) \n";
                print OUTPUT "// $pyFunc -> $ret $cfunc (@params ) \n";
                # start our function
                print OUTPUT "static tp_obj ".$glueprefix.$f."(TP)\n {\n";

                debug("$f invoked\n");
                 print OUTPUT "  tp_obj self = tp_getraw(tp);\n";  # We need a this as we only deal with instance!

				if($staticClass == 1)
				{
					print OUTPUT "  IScriptEngine *engine = (IScriptEngine*)tp_get(tp, tp->builtins, tp_string(\"userdata\")).data.val;\n";
					print OUTPUT "  IEditor *editor = engine->editor();\n";
                }

                 print OUTPUT "  TinyParams pm(tp);\n";
                 print OUTPUT "  $cookieName *me = ($cookieName *)pm.asThis(&self, $cookieId);\n\n";

                if($params[0]=~m/^void$/)
                {
                        $nb=0;
                }
                # unmarshall params...
                if($nb)
                {
                         for($i=0;$i<$nb;$i++)
                         {
                                 genParam($i,$params[$i]);
                         }
                }
                # call function
                if(!($ret=~m/^void$/))
                {
                        if($ret=~m/str/)
                        {
                                print OUTPUT "  char *r = ";
                        }else
                        {
                                print OUTPUT "  ".$ret." r = ";
                        }
                }
                if($staticClass==1)
                {
                        print OUTPUT "  ".$functionPrefix.$cfunc."(";
                }else
                {
                        print OUTPUT "  me->".$functionPrefix.$cfunc."(";
                }
                for($i=0;$i<$nb;$i++)
                {
                        if($i)
                        {
                                print OUTPUT ",";
                        }
                        print OUTPUT "p".$i;
                }
                print OUTPUT "); \n";
                # return value (if any)
                print OUTPUT genReturn($ret);
                print OUTPUT "\n}\n";

        }
        genGetSet();

}
#
#
#
sub genCtorDtor
{
   my $dtor=  "myDtor".$className;
# Dtor
        print OUTPUT "// Dctor\n";
        print OUTPUT "static void ".$dtor."(tp_vm *vm,tp_obj self)\n";
        print OUTPUT "{\n";
        debug("$dtor invoked\n");
        if($staticClass==1)
        {
                # no cookie, so no need to destroy
        }else
        {
                print OUTPUT "  $cookieName *cookie = ($cookieName *)self.data.val;\n";
                print OUTPUT "  if (cookie) delete cookie;\n";
                print OUTPUT "  self.data.val = NULL;\n";
        }
        print OUTPUT "}\n";
        # end
#
# Ctor
#
        # ctor
        print OUTPUT "// Ctor (@ctorParams)\n";
        print OUTPUT "static tp_obj myCtor".$className."(tp_vm *vm)\n";
        print OUTPUT "{\n";
        debug("ctor of $className invoked\n");
        print OUTPUT "  tp_obj self = tp_getraw(vm);\n";
        print OUTPUT "  TinyParams pm(vm);\n";

        if($staticClass==1)
        {
                print OUTPUT "  void *me = NULL;\n";
        }else
        {

                my $i;
                my $nb=scalar(@ctorParams);
                for($i=0;$i<$nb;$i++)
                         {
                                 genParam($i,$ctorParams[$i]);
                         }
                print OUTPUT "  $cookieName *me = new $cookieName(";
                for($i=0;$i<$nb;$i++)
                {
                        if($i)
                        {
                                print OUTPUT ",";
                        }
                        print OUTPUT "p".$i;
                }
                print OUTPUT ");\n";
        }
        print OUTPUT "  tp_obj cdata = tp_data(vm, $cookieId, me);\n";
        print OUTPUT "  cdata.data.info->xfree = $dtor;\n";
        print OUTPUT "  tp_set(vm, self, tp_string(\"cdata\"), cdata);\n";
        print OUTPUT "  return tp_None;\n";
        print OUTPUT "}\n";
        # end
}
#
#  Generate help table
#
sub genTables
{
my $helpName=$glueprefix."_".$className."_help";
my $f;
my $cfunk;
my $pyFunc;
                print OUTPUT "static tp_obj ".$helpName."(TP)\n{\n";
                print OUTPUT "\tPythonEngine *engine = (PythonEngine*)tp_get(tp, tp->builtins, tp_string(\"userdata\")).data.val;\n\n";

                foreach $f(  keys %cFuncs)
                {
                        my @params=@{$funcParams{$f}};
                        print OUTPUT "\tengine->callEventHandlers(IScriptEngine::Information, NULL, -1, \"$f(".join(",",@params) .")\\n\");\n";
                }
                print OUTPUT "\n\treturn tp_None;\n";
                print OUTPUT "};\n";
#
#  Create the init function that will register our class
#
        my $setName=$glueprefix."_".$className."_set";
        my $getName=$glueprefix."_".$className."_get";

        print OUTPUT "tp_obj initClass".$className."(tp_vm *vm)\n";
        print OUTPUT "{\n";
        print OUTPUT "  tp_obj myClass = tp_class(vm);\n";
        print OUTPUT "  tp_set(vm,myClass, tp_string(\"__init__\"), tp_fnc(vm,myCtor".$className."));\n";
        print OUTPUT "  tp_set(vm,myClass, tp_string(\"__set__\"), tp_fnc(vm,".$setName."));\n";
        print OUTPUT "  tp_set(vm,myClass, tp_string(\"__get__\"), tp_fnc(vm,".$getName."));\n";
        print OUTPUT "  tp_set(vm,myClass, tp_string(\"help\"), tp_fnc(vm,$helpName));\n";
        print OUTPUT "  return myClass;\n";
        print OUTPUT "}\n";
}
##################################
#  Main
##################################
if(scalar(@ARGV)!=2)
{
        die("admPy inputfile outputfile\n");
}
$input=$ARGV[0];
$output=$ARGV[1];
##
## Main Loop
##
# 1 grab all functions
parseFile($input);
# 2 gen glue
open(OUTPUT,">$output") or die("Cannot open $output");
print OUTPUT "// Generated by admPyClass.pl do not edit !\n";
genGlue();
#
#
#
genCtorDtor();
#
# 3 gen tables (ctor, help, register)
#
genTables();
close(OUTPUT);


#
print "done\n.";
