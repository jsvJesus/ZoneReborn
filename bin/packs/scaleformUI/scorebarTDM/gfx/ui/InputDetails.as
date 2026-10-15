class gfx.ui.InputDetails
{
   var type;
   var code;
   var value;
   var navEquivalent;
   var controllerIdx;
   function InputDetails(type, code, value, navEquivalent, controllerIdx)
   {
      this.type = type;
      this.code = code;
      this.value = value;
      this.navEquivalent = navEquivalent;
      this.controllerIdx = controllerIdx;
   }
   function toString()
   {
      return ["[InputDelegate","code=" + this.code,"type=" + this.type,"value=" + this.value,"navEquivalent=" + this.navEquivalent,"controllerIdx=" + this.controllerIdx + "]"].toString();
   }
}
