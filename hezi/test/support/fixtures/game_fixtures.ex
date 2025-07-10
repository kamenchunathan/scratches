defmodule Hezi.GameFixtures do
  @moduledoc """
  This module defines test helpers for creating
  entities via the `Hezi.Game` context.
  """

  @doc """
  Generate a index.
  """
  def index_fixture(attrs \\ %{}) do
    {:ok, index} =
      attrs
      |> Enum.into(%{
        room: "some room"
      })
      |> Hezi.Game.create_index()

    index
  end
end
