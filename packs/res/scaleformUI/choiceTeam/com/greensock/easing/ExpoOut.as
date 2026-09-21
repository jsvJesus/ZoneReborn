class com.greensock.easing.ExpoOut extends com.greensock.easing.Ease
{
   static var ease = new com.greensock.easing.ExpoOut();
   function ExpoOut()
   {
      super();
   }
   function getRatio(p)
   {
      return 1 - Math.pow(2,-10 * p);
   }
}
