class gfx.ui.InputDetails
{
   var type;
   var code;
   var value;
   var navEquivalent;
   function InputDetails(type, code, value, navEquivalent)
   {
      this.type = type;
      this.code = code;
      this.value = value;
      this.navEquivalent = navEquivalent;
   }
   function toString()
   {
      return ["[InputDelegate","code=" + this.code,"type=" + this.type,"value=" + this.value,"navEquivalent=" + this.navEquivalent + "]"].toString();
   }
}
