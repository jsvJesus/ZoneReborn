class com.scaleform.scrolerdListDM extends MovieClip
{
   function scrolerdListDM()
   {
      super();
      trace("scrolerdListDM constuctor");
   }
   function init()
   {
      trace("created");
   }
   function onLoad()
   {
      trace("scrolerdListDM onLoad1, dataProvider:");
   }
   function select(data)
   {
      trace("scrolerdListDM select");
   }
   function foo()
   {
      trace("scrolerdListDM foo");
      var _loc3_ = 0;
      while(_loc3_ < 12)
      {
         var _loc2_ = this["item" + (_loc3_ + 1)];
         if(!_loc2_._origY)
         {
            _loc2_._origY = _loc2_._y;
         }
         _loc2_._y = _loc2_._origY;
         _loc2_._alpha = 100;
         _loc2_.tweenFrom(1 + _loc3_ / 12,{_y:10,_alpha:0},mx.transitions.easing.Bounce.easeIn);
         _loc3_ = _loc3_ + 1;
      }
   }
   function snake()
   {
      var _loc3_ = 0;
      while(_loc3_ < 12)
      {
         var _loc2_ = this["item" + (_loc3_ + 1)];
         if(!_loc2_._origX)
         {
            _loc2_._origX = _loc2_._x;
         }
         if(!_loc2_._origY)
         {
            _loc2_._origY = _loc2_._y;
         }
         _loc2_._y = _loc2_._origY;
         _loc2_._x = _loc2_._origX;
         _loc2_._alpha = 100;
         _loc2_.tweenFrom(1 + _loc3_ / 12,{_x:50,_alpha:0},mx.transitions.easing.Bounce.easeOut);
         _loc3_ = _loc3_ + 1;
      }
   }
   function setData(players)
   {
   }
}
