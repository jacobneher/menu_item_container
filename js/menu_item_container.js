(function ($) {
  Drupal.behaviors.menu_item_container = {
    attach: function(context, settings) {
      
      $('ul.sf-menu a.menu-item-container').click(function(e) {
				e.preventDefault();
      });
		}
  }
})(jQuery);