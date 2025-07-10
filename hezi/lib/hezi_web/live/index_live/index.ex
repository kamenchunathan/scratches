defmodule HeziWeb.IndexLive.Index do
  use HeziWeb, :live_view

  alias Hezi.Game

  @impl true
  def render(assigns) do
    ~H"""
    <Layouts.app flash={@flash}>
      <.header>Guess a number</.header>
      <h1>Current score: {@score}</h1>
      <div class="flex flex-row justify-evenly">
        <%= for n <- 1..10 do %>
          <a
            href="#"
            phx-click="guess"
            phx-value-number={n}
            class="bg-slate-700 shadow py-1 px-2 mx-2 rounded-md text-white text-lg"
          >
            {n}
          </a>
        <% end %>
      </div>
    </Layouts.app>
    """
  end

  @impl true
  def mount(_params, _session, socket) do
    {:ok,
     socket
     |> assign(:page_title, "Listing Game")
     |> assign(:score, 0)
     |> stream(:game, Game.list_game())}
  end

  @impl true
  def handle_event("guess", %{"number" => guess}, socket) do
    score = socket.assigns.score - 1
    {:noreply, assign(socket, :score, score)}
  end
end
