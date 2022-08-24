<script lang="ts">
  export let status: Array<number>;
</script>

<main>
  <ul class="water-level-container">
    <li class="water-bottle">
      <div class="cap">
        <div class="cap-top" />
        <div class="cap-seal" />
      </div>
      <div class="bottle bottle-1">
        <div id="lvl" class="water-low" class:water-selected={status[0]} />
      </div>
    </li>

    <li class="water-bottle">
      <div class="cap">
        <div class="cap-top" />
        <div class="cap-seal" />
      </div>
      <div class="bottle bottle-2">
        <div id="lvl" class="water-medium " class:water-selected={status[1]}/>
      </div>
    </li>

    <li class="water-bottle">
      <div class="cap">
        <div class="cap-top" />
        <div class="cap-seal" />
      </div>
      <div class="bottle bottle-3">
        <div id="lvl" class="water-mid-full" class:water-selected={status[2]}/>
      </div>
    </li>

    <li class="water-bottle">
      <div class="cap">
        <div class="cap-top" />
        <div class="cap-seal" />
      </div>
      <div class="bottle bottle-4">
        <div id="lvl" class="water-full " class:water-selected={status[3]} />
      </div>
    </li>
  </ul>
</main>

<style>
  .water-level-container {
    font-size: 1.5rem;

    display: flex;
    flex-direction: row;
    justify-content: center;
  }

  li.water-bottle {
    float: left;
    margin-right: 15px;
  }

  ul {
    list-style-type: none;
  }

  .bottle {
    background: #ddd;
    width: 100px;
    border-radius: 30px 30px 6px 6px;
    height: 200px;
    padding: 6px;
    box-shadow: 3px 3px 3px #efefef;
    margin-bottom: 4px;
    display: flex;
    flex-direction: column;
    justify-content: end;
  }
  .bottle:hover {
    background: #eee;
    box-shadow: 2px 2px 2px #efefef;
  }
  .cap {
    background: #ddd;
    margin: 0 25px;
    width: 60px;
    border-radius: 8px 8px 10px 12px;
    height: 35px;
    border-top: 1px solid #ddd;
    border-bottom: 2px dotted #efefef;
  }

  .cap-top {
    background: #ededed;
    height: 6px;
    border-radius: 10px 10px 0 0;
  }

  .cap-seal {
    margin-top: 20px;
    background: #ccc;
    height: 6px;
    border-radius: 0 0 10px 10px;
  }

  .water-full {
    background: lightgray;
    width: 100px;
    margin-top: 10px;
    height: 190px;
    z-index: 1;
    border-radius: 28px 28px 10px 10px;
  }
  .water-mid-full {
    background: lightgray;
    width: 100px;
    margin-top: 10px;
    height: 150px;
    z-index: 1;
    border-radius: 28px 28px 10px 10px;
  }

  .water-medium {
    background: lightgray;
    width: 100px;
    height: 120px;
    margin-top: 80px;
    z-index: 1;
    border-radius: 28px 28px 10px 10px;
  }

  .water-low {
    background: lightgray;
    width: 100px;
    height: 75px;
    margin-top: 125px;
    z-index: 1;
    border-radius: 28px 28px 10px 10px;
  }

  .water-empty {
    background: lightgray;
    width: 100px;
    height: 15px;
    margin-top: 185px;
    z-index: 1;
    border-radius: 28px 28px 10px 10px;
  }

  .water-selected {
    background: lightblue !important;
  }
</style>
