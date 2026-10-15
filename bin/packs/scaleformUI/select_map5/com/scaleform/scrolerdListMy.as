class com.scaleform.scrolerdListMy extends MovieClip
{
   function scrolerdListMy()
   {
      super();
   }
   function onLoad()
   {
   }
   function select(data)
   {
   }
   function foo()
   {
      var i = 0;
      while(i < 12)
      {
         var mc = this["item" + (i + 1)];
         if(!mc._origY)
         {
            mc._origY = mc._y;
         }
         mc._y = mc._origY;
         mc._alpha = 100;
         mc.tweenFrom(1 + i / 12,{_y:10,_alpha:0},mx.transitions.easing.Bounce.easeIn);
         i++;
      }
   }
   function snake()
   {
      var i = 0;
      while(i < 12)
      {
         var mc = this["item" + (i + 1)];
         if(!mc._origX)
         {
            mc._origX = mc._x;
         }
         if(!mc._origY)
         {
            mc._origY = mc._y;
         }
         mc._y = mc._origY;
         mc._x = mc._origX;
         mc._alpha = 100;
         mc.tweenFrom(1 + i / 12,{_x:50,_alpha:0},mx.transitions.easing.Bounce.easeOut);
         i++;
      }
   }
   function setData(mapdata)
   {
   }
}
