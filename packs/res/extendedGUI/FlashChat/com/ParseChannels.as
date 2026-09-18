package com
{
   public class ParseChannels
   {
      public function ParseChannels()
      {
         super();
      }
      
      public function parse(a:Array, defaultChannals:Array) : Array
      {
         var Obj:Object = null;
         var j:* = undefined;
         var arr:Array = new Array();
         for(var i:* = 0; i < defaultChannals.length; i++)
         {
            Obj = new Object();
            Obj.id = defaultChannals[i].id;
            Obj.label = defaultChannals[i].label;
            Obj.selected = defaultChannals[i].selected;
            Obj.com = defaultChannals[i].com;
            for(j = 0; j < a.length; j++)
            {
               if(defaultChannals[i].id == a[j])
               {
                  Obj.selected = true;
               }
            }
            arr.push(Obj);
         }
         return arr;
      }
      
      public function unparse(AR:Array) : Array
      {
         var arr:Array = new Array();
         for(var i:* = 0; i < AR.length; i++)
         {
            if(AR[i].selected == true)
            {
               if(AR[i].id != -99)
               {
                  arr.push(AR[i].id);
               }
            }
         }
         return arr;
      }
   }
}

