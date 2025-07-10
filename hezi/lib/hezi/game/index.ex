defmodule Hezi.Game.Index do
  use Ecto.Schema
  import Ecto.Changeset

  schema "game" do
    field :room, :string

    timestamps(type: :utc_datetime)
  end

  @doc false
  def changeset(index, attrs) do
    index
    |> cast(attrs, [:room])
    |> validate_required([:room])
  end
end
