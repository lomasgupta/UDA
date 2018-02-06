<?php

/**
 * @file
 * Definition of commerce_shs_handler_filter_term_node_tid_depth.
 */

/**
 * Filter by term id (including selection by simple hierarchical select).
 * This is a combination of 
 *   shs_handler_filter_term_node_tid and
 *   shs_handler_filter_term_node_tid_depth
 * It is needed to propagate the search to depth, but without using the 
 * taxonomy_index table.
 *
 * @ingroup views_filter_handlers
 */
class commerce_shs_handler_filter_term_node_tid_depth extends shs_handler_filter_term_node_tid {

  function query() {
    // Check if multiple select allowed.
    if ($this->options['expose']['multiple']) {
      // If no filter values are present, then do nothing.
      if ($this->value == 'All') {
        return;
      }
      else {
        $this->value = explode('+', $this->value[0]);
        $this->value = explode(',', end($this->value));
        $operator = 'IN';
      }
    }
    else {
      // If no filter values are present, then do nothing.
      if (count($this->value) == 0) {
        return;
      }
      elseif (count($this->value) == 1) {
        // $this->value is an array with a single element so convert it.
        if (is_array($this->value)) {
          $this->value = current($this->value);
        }
        $operator = '=';
      }
      else {
        $operator = 'IN';
      }
    }

    // The normal use of ensure_my_table() here breaks Views.
    // So instead we trick the filter into using the alias of the base table.
    // See http://drupal.org/node/271833
    // If a relationship is set, we must use the alias it provides.
    if (!empty($this->relationship)) {
      $this->table_alias = $this->relationship;
    }
    // If no relationship, then use the alias of the base table.
    elseif (isset($this->query->table_queue[$this->query->base_table]['alias'])) {
      $this->table_alias = $this->query->table_queue[$this->query->base_table]['alias'];
    }
    // This should never happen, but if it does, we fail quietly.
    else {
      return;
    }

    // Need to join commerce_product query to field_data_field_product_category
    $join = new views_join();
    $join->construct($this->options['table'], $this->table_alias, 'product_id', 'entity_id', array(), 'INNER');
    $this->query->add_relationship($this->options['table'], $join, $this->table_alias, $this->relationship);

    // Now build the subqueries.
    $subquery = db_select('taxonomy_term_hierarchy', 'th0');
    $subquery->addField('th0', 'tid');
    $where = db_or()->condition('th0.tid', $this->value, $operator);
    $where->condition('th0.parent', $this->value, $operator);
    $last = "th0";
    // Search 10 levels of hierarchy, might be better to make this a user setting
    foreach (range(1, 9) as $count) {
      $subquery->leftJoin('taxonomy_term_hierarchy', "th$count", "th$count.tid = $last.parent");
      $where->condition("th$count.parent", $this->value, $operator);
      $last = "th$count";
    }

    $subquery->condition($where);
    $this->query->add_where($this->options['group'], $this->options['table'] .'.'. $this->real_field, $subquery, 'IN');
  }

}
