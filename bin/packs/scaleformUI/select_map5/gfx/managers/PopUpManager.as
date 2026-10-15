class gfx.managers.PopUpManager
{
   static var index = 0;
   function PopUpManager()
   {
   }
   static function createPopUp(context, linkage, initProperties, relativeTo)
   {
      var targetContainer = context._parent;
      if(!targetContainer)
      {
         targetContainer = context;
      }
      var depth = targetContainer.getNextHighestDepth();
      var baseProps = {_x:initProperties._x,_y:initProperties._y};
      if(relativeTo != null)
      {
         var p = {x:initProperties._x,y:initProperties._y};
         relativeTo.localToGlobal(p);
         targetContainer.globalToLocal(p);
         if(initProperties._x != null)
         {
            initProperties._x = p.x;
         }
         if(initProperties._y != null)
         {
            initProperties._y = p.y;
         }
      }
      var clip = targetContainer.attachMovie(linkage,"popup" + gfx.managers.PopUpManager.index++,depth,initProperties);
      if(!clip)
      {
         initProperties._x = baseProps._x;
         initProperties._y = baseProps._y;
         clip = gfx.managers.PopUpManager.createPopUpRetry(context,linkage,initProperties,relativeTo);
         if(clip)
         {
         }
         return clip;
      }
      clip.topmostLevel = true;
      return clip;
   }
   static function destroyPopUp(popUp)
   {
      popUp.removeMovieClip();
   }
   static function movePopUp(context, popUp, relativeTo, x, y)
   {
      if(popUp == null)
      {
         return undefined;
      }
      var targetContainer = context._parent;
      if(!targetContainer)
      {
         targetContainer = context;
      }
      var p = {x:x,y:y};
      relativeTo.localToGlobal(p);
      targetContainer.globalToLocal(p);
      popUp._x = p.x;
      popUp._y = p.y;
   }
   static function centerPopUp(popUp)
   {
      if(popUp == null)
      {
         return undefined;
      }
      popUp._x = Stage.width - popUp._width >> 1;
      popUp._y = Stage.height - popUp._height >> 1;
   }
   static function createPopUpRetry(context, linkage, initProperties, relativeTo)
   {
      var depth = context.getNextHighestDepth();
      if(relativeTo != null)
      {
         var p = {x:initProperties._x,y:initProperties._y};
         relativeTo.localToGlobal(p);
         context.globalToLocal(p);
         if(initProperties._x != null)
         {
            initProperties._x = p.x;
         }
         if(initProperties._y != null)
         {
            initProperties._y = p.y;
         }
      }
      var clip = context.attachMovie(linkage,"popup" + gfx.managers.PopUpManager.index++,depth,initProperties);
      clip.topmostLevel = true;
      return clip;
   }
}
