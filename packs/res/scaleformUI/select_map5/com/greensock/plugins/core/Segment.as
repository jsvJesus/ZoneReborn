class com.greensock.plugins.core.Segment
{
   var a;
   var b;
   var c;
   var d;
   var da;
   var ca;
   var ba;
   function Segment(a, b, c, d)
   {
      this.a = a;
      this.b = b;
      this.c = c;
      this.d = d;
      this.da = d - a;
      this.ca = c - a;
      this.ba = b - a;
   }
}
