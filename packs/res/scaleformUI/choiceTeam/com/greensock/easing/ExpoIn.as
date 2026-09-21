class com.greensock.easing.ExpoIn extends com.greensock.easing.Ease
{
   static var ease = new com.greensock.easing.ExpoIn();
   function ExpoIn()
   {
      super();
   }
   function getRatio(p)
   {
      return Math.pow(2,10 * (p - 1)) - 0.001;
   }
}
