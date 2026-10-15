class gfx.controls.ButtonGroup extends gfx.events.EventDispatcher
{
   var scope;
   var children;
   var selectedButton;
   var name = "buttonGroup";
   function ButtonGroup(name, scope)
   {
      super();
      this.name = name;
      this.scope = scope;
      this.children = [];
   }
   function get length()
   {
      return this.children.length;
   }
   function addButton(button)
   {
      if(this.indexOf(button) > -1)
      {
         return undefined;
      }
      this.children.push(button);
      if(button.selected)
      {
         this.setSelectedButton(button);
      }
      button.addEventListener("select",this,"handleSelect");
      button.addEventListener("click",this,"handleClick");
   }
   function removeButton(button)
   {
      var index = this.indexOf(button);
      if(index > -1)
      {
         this.children.splice(index,1);
         button.removeEventListener("select",this,"handleSelect");
         button.removeEventListener("click",this,"handleClick");
      }
      if(this.selectedButton == button)
      {
         this.selectedButton = null;
      }
   }
   function indexOf(button)
   {
      var l = this.length;
      if(l == 0)
      {
         return -1;
      }
      var i = 0;
      while(i < this.length)
      {
         if(this.children[i] == button)
         {
            return i;
         }
         i++;
      }
      return -1;
   }
   function getButtonAt(index)
   {
      return this.children[index];
   }
   function get data()
   {
      return this.selectedButton.data;
   }
   function setSelectedButton(button)
   {
      if(this.selectedButton == button || this.indexOf(button) == -1 && button != null)
      {
         return undefined;
      }
      if(this.selectedButton != null && this.selectedButton._name != null)
      {
         this.selectedButton.selected = false;
      }
      this.selectedButton = button;
      if(this.selectedButton == null)
      {
         return undefined;
      }
      this.selectedButton.selected = true;
      this.dispatchEvent({type:"change",item:this.selectedButton,data:this.selectedButton.data});
   }
   function toString()
   {
      return "[Scaleform RadioButtonGroup " + this.name + "]";
   }
   function handleSelect(event)
   {
      if(event.target.selected)
      {
         this.setSelectedButton(event.target);
      }
      else
      {
         this.setSelectedButton(null);
      }
   }
   function handleClick(event)
   {
      this.dispatchEvent({type:"itemClick",item:event.target});
      this.setSelectedButton(event.target);
   }
}
