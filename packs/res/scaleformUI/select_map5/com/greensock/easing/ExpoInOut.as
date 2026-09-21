class com.greensock.easing.ExpoInOut extends com.greensock.easing.Ease
{
   static var ease = new com.greensock.easing.ExpoInOut();
   function ExpoInOut()
   {
      super();
   }
   function getRatio(p)
   {
      return (p *= 2) >= 1 ? 0.5 * (2 - Math.pow(2,-10 * (p - 1))) : 0.5 * Math.pow(2,10 * (p - 1));
   }
}
