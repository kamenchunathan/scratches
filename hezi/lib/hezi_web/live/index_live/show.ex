defmodule HeziWeb.IndexLive.Show do
  use HeziWeb, :live_view

  alias Hezi.Game

  @impl true
  def render(assigns) do
    ~H"""
    <Layouts.app flash={@flash}>
      <.header>
        Index {@index.id}
        <:subtitle>This is a index record from your database.</:subtitle>
        <:actions>
          <.button navigate={~p"/game"}>
            <.icon name="hero-arrow-left" />
          </.button>
          <.button variant="primary" navigate={~p"/game/#{@index}/edit?return_to=show"}>
            <.icon name="hero-pencil-square" /> Edit index
          </.button>
        </:actions>
      </.header>

      <.list>
        <:item title="Room">{@index.room}</:item>
      </.list>
    </Layouts.app>
    """
  end

  @impl true
  def mount(%{"id" => id}, _session, socket) do
    {:ok,
     socket
     |> assign(:page_title, "Show Index")
     |> assign(:index, Game.get_index!(id))}
  end
end
